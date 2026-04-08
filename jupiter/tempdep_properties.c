
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <inttypes.h>

#include "geometry/defs.h"
#include "tempdep_properties.h"
#include "tempdep_calc.h"
#include "csv.h"
#include "csvutil.h"
#include "geometry/infomap.h"
#include "geometry/variant.h"
#include "os/asprintf.h"

#include "table/table.h"

enum tempdep_set_error
{
  TEMPDEP_SET_ERROR_SUCCESS, ///< No error
  TEMPDEP_SET_ERROR_NOMEM,   ///< Memory allocation failure
  TEMPDEP_SET_ERROR_RANGE,   ///< Invalid value, or out of range
  TEMPDEP_SET_ERROR_NSPIECE, ///< Not supported as a piece of piecewise function
};

/**
 * @brief Prototype for defining tempdep_property from CSV data
 * @param data Pointer to data to set
 * @param csv CSV data should be read from
 * @param fname Filename of @p csv data comes from
 * @param found_row The row of CSV should be read from
 * @param start The start column of CSV should be read from, and
 *              should return the last read column
 * @param dom The domain for piecewise function, NULL if not piecewise function
 */
typedef enum tempdep_set_error tempdep_set_csv(union tempdep_property_u *data,
                                               csv_data *csv, const char *fname,
                                               csv_row *found_row,
                                               csv_column **start,
                                               struct tempdep_domain_data *dom);

typedef void tempdep_dealloc(union tempdep_property_u *data);

/**
 * @brief Prototype for defining information map for CSV data
 * @param map Map to generate onto.
 * @param data Data to map
 * @param domain_is_local 0 if domain information is externally controlled,
 *                        (i.e., piecewise function), otherwise non-0 will be
 *                        passed.
 */
typedef geom_error tempdep_infomap_gen(geom_info_map *map,
                                       union tempdep_property_u *data,
                                       int domain_is_local);

struct tempdep_property_funcs
{
  tempdep_set_csv *setter;          ///< Setter from CSV
  tempdep_dealloc *deallocator;     ///< Deallocator
  tempdep_infomap_gen *infomap_gen; ///< Infomap generator
};

static const struct tempdep_property_funcs *
tempdep_get_func(tempdep_property_type type);

static void tempdep_domain_set_csv(struct tempdep_domain_data *dom,
                                   csv_data *csv, const char *fname,
                                   csv_row *found_row, csv_column **start,
                                   enum tempdep_set_error *ret)
{
  SET_P_INIT(csv, fname, &found_row, start);

  CSVASSERT(start);
  CSVASSERT(dom);

  SET_P_NEXT(&dom->minT, exact_double, 0.0);
  if (!SET_P_PERROR_FINITE(dom->minT, ERROR,
                           "Minimum domain value must be finite")) {
    if (ret)
      *ret = TEMPDEP_SET_ERROR_RANGE;
  }

  SET_P_NEXT(&dom->maxT, exact_double, 5000.0);
  if (!SET_P_PERROR_GREATER(dom->maxT, dom->minT, OFF, OFF, ERROR,
                            "Maximum domain value must be greater than"
                            " minimum (and finite)")) {
    if (ret)
      *ret = TEMPDEP_SET_ERROR_RANGE;
  }
}

static void tempdep_domain_infomap(struct tempdep_domain_data *dom,
                                   geom_info_map *head, geom_variant *var,
                                   geom_error *err)
{
  CSVASSERT(dom);
  CSVASSERT(head);
  CSVASSERT(var);

  geom_variant_set_double(var, dom->minT);
  geom_info_map_append(head, var, "Minimum domain", "L", err);

  geom_variant_set_double(var, dom->maxT);
  geom_info_map_append(head, var, "Maximum domain", "L", err);
}

static enum tempdep_set_error
tempdep_const_set_csv(union tempdep_property_u *data, csv_data *csv,
                      const char *fname, csv_row *found_row, csv_column **start,
                      struct tempdep_domain_data *dom)
{
  struct tempdep_const_data *cdata;
  SET_P_INIT(csv, fname, &found_row, start);

  enum tempdep_set_error ret = TEMPDEP_SET_ERROR_SUCCESS;

  CSVASSERT(start);

  cdata = &data->t_const;

  SET_P_NEXT(&cdata->value, exact_double, NAN);
  if (!SET_P_PERROR_FINITE(cdata->value, ERROR, "Const value must be finite")) {
    ret = TEMPDEP_SET_ERROR_RANGE;
  }

  return ret;
}

static geom_error tempdep_const_infomap_gen(geom_info_map *map,
                                            union tempdep_property_u *data,
                                            int domain_is_local)
{
  struct tempdep_const_data *cdata;
  geom_variant *var;
  geom_error err;

  cdata = &data->t_const;

  err = GEOM_SUCCESS;
  var = geom_variant_new(&err);
  if (!var) {
    return err;
  }

  geom_variant_set_double(var, cdata->value);

  /*
   * See `tempdep_property_create_info_map()` for more info for the unit.
   *
   * Usually, only "I", "L" or other fixed units should be used for
   * the unit. Mixing "I" and "L" may confuse the user. For example,
   * the unit of the coefficient of T in polynomial can be "I/L", and
   * "I/L" will transform to "kg/m3/K" (for density). The user may
   * believe "I must find a value united of 'kg/m3/K' in the
   * references or database."
   *
   * (note: "L" is the length in the geometry library, and explicitly
   *  required. This means that the unit of temperature must not be
   *  unitless.)
   */
  geom_info_map_append(map, var, "Constant value", "I", &err);

  geom_variant_delete(var);
  return err;
}

static const struct tempdep_property_funcs tempdep_const_funcs = {
  .setter = tempdep_const_set_csv,
  .deallocator = NULL,
  .infomap_gen = tempdep_const_infomap_gen,
};

static enum tempdep_set_error
tempdep_table_set_csv(union tempdep_property_u *data, csv_data *csv,
                      const char *fname, csv_row *found_row, csv_column **start,
                      struct tempdep_domain_data *dom)
{
  struct tempdep_table_data *tdata;
  enum tempdep_set_error ret = TEMPDEP_SET_ERROR_SUCCESS;
  int nel;
  table_error terr;
  static const double yd[] = {0.0};
  SET_P_INIT(csv, fname, &found_row, start);

  CSVASSERT(start);

  tdata = &data->t_table;

  tdata->data = table_alloc();
  if (!tdata->data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    ret = TEMPDEP_SET_ERROR_NOMEM;
  }

  SET_P_NEXT(&nel, int, 0);
  if (nel <= 0) {
    SET_P_PERROR(ERROR, "Number of elements must be positive");
    ret = TEMPDEP_SET_ERROR_RANGE;
  } else {
    int i;
    double *xd;
    double *td;
    double val;
    double last_temp;

    xd = malloc(sizeof(double) * nel);
    td = malloc(sizeof(double) * nel);
    if (!xd || !td) {
      free(xd);
      free(td);
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      xd = NULL;
      td = NULL;
      ret = TEMPDEP_SET_ERROR_NOMEM;
    }

    last_temp = 0.0;
    for (i = 0; i < nel; ++i) {
      SET_P_NEXT(&val, exact_double, NAN);
      if (!SET_P_PERROR_FINITE(val, ERROR, "Temperature must be finite")) {
        ret = TEMPDEP_SET_ERROR_RANGE;
      } else {
        if (i > 0 && val <= last_temp) {
          SET_P_PERROR(ERROR, "Temperature must be in ascending order");
          ret = TEMPDEP_SET_ERROR_RANGE;
        }
        if (dom) {
          if (i == 0 && val != dom->minT) {
            SET_P_PERROR(ERROR, "First temperature should be equal to the "
                                "lower domain value of the piece");
            ret = TEMPDEP_SET_ERROR_RANGE;
          }
          if (i == nel - 1 && val != dom->maxT) {
            SET_P_PERROR(ERROR, "Last temperature should be equal to the "
                                "upper domain value of the piece");
            ret = TEMPDEP_SET_ERROR_RANGE;
          }
        }
      }
      if (xd) {
        xd[i] = val;
      }
      last_temp = val;

      SET_P_NEXT(&val, exact_double, NAN);
      if (!SET_P_PERROR_FINITE(val, ERROR, "Property value must be finite")) {
        ret = TEMPDEP_SET_ERROR_RANGE;
      }
      if (td) {
        td[i] = val;
      }
    }

    if (xd && td) {
      terr = table_init(tdata->data, NULL, TABLE_GEOMETRY_RECTILINEAR, nel, 1,
                        TABLE_INTERP_LINEAR, xd, yd, td);
      free(xd);
      free(td);

      if (terr != TABLE_SUCCESS) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, "%s",
                   table_errorstr(terr));
        ret = 1;
        table_free(tdata->data);
        tdata->data = NULL;
      }

      if (tdata->data) {
        if (nel >= 1000) {
          terr = table_set_algorithm(tdata->data, TABLE_SALG_BIN_TREE_MINMAX);
        } else {
          terr = table_set_algorithm(tdata->data, TABLE_SALG_LINEAR_SAVE);
        }
        if (terr != TABLE_SUCCESS) {
          csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, "%s",
                     table_errorstr(terr));
          ret = TEMPDEP_SET_ERROR_RANGE;
          table_free(tdata->data);
          tdata->data = NULL;
        }
      }
    }
  }

  return ret;
}

static void tempdep_table_deallocate(union tempdep_property_u *data)
{
  if (data->t_table.data) {
    table_free(data->t_table.data);
  }
}

static geom_error tempdep_table_infomap_gen(geom_info_map *map,
                                            union tempdep_property_u *data,
                                            int domain_is_local)
{
  struct tempdep_table_data *tdata;
  geom_variant *var;
  geom_error err;

  tdata = &data->t_table;

  err = GEOM_SUCCESS;
  var = geom_variant_new(&err);
  if (!var) {
    return err;
  }

  if (tdata->data) {
    table_size ti, tsz;
    const double *td, *dd;

    tsz = table_get_nx(tdata->data);

    geom_variant_set_size_value(var, tsz);
    geom_info_map_append(map, var, "Number of points", "", &err);

    td = table_get_xdata(tdata->data);
    dd = table_get_data(tdata->data);

    for (ti = 0; ti < tsz; ++ti) {
      char *sp;
      int r;

      r = jupiter_asprintf(&sp, "Temperature of Point %" PRIdMAX "",
                           (intmax_t)ti);
      geom_variant_set_double(var, td[ti]);
      if (r >= 0) {
        geom_info_map_append(map, var, sp, "L", &err);
        free(sp);
      } else {
        geom_info_map_append(map, var, "Temperature of point", "L", &err);
      }

      r =
        jupiter_asprintf(&sp, "Data value of Point %" PRIdMAX "", (intmax_t)ti);
      geom_variant_set_double(var, dd[ti]);
      if (r >= 0) {
        geom_info_map_append(map, var, sp, "I", &err);
        free(sp);
      } else {
        geom_info_map_append(map, var, "Data value of point", "L", &err);
      }
    }
  } else {
    geom_variant_nullify(var);
    geom_info_map_append(map, var, "Table data could not be read", "", &err);
  }

  geom_variant_delete(var);
  return err;
}

static const struct tempdep_property_funcs tempdep_table_funcs = {
  .setter = tempdep_table_set_csv,
  .deallocator = tempdep_table_deallocate,
  .infomap_gen = tempdep_table_infomap_gen,
};

static enum tempdep_set_error
tempdep_poly_set_csv(union tempdep_property_u *data, csv_data *csv,
                     const char *fname, csv_row *found_row, csv_column **start,
                     struct tempdep_domain_data *dom)
{
  struct tempdep_poly_data *pdata;
  enum tempdep_set_error ret = TEMPDEP_SET_ERROR_SUCCESS;
  ptrdiff_t npow;
  SET_P_INIT(csv, fname, &found_row, start);

  CSVASSERT(start);

  pdata = &data->t_poly;

  SET_P_NEXT(&pdata->polymax, int, 0);
  if (pdata->polymax <= 0) {
    SET_P_PERROR(ERROR, "Maximum power must be positive");
    if (pdata->polymax == 0) {
      SET_P_PERROR(INFO,
                   "(Use CONST instead of giving 0 for maximum power of POLY)");
    }
    ret = TEMPDEP_SET_ERROR_SUCCESS;
  }

  npow = pdata->polymax;
  npow = npow + 1;

  if (ret == 0 && npow < 0) {
    SET_P_PERROR(ERROR, "Too large maximum power");
    pdata->polymax = -1;
    ret = TEMPDEP_SET_ERROR_RANGE;
  }

  if (dom) {
    pdata->dom = *dom;
  } else {
    tempdep_domain_set_csv(&pdata->dom, csv, fname, found_row, start, &ret);
  }

  pdata->vec = NULL;
  if (npow > 0) {
    ptrdiff_t i;
    double arg;

    pdata->vec = (double *)calloc(sizeof(double), npow);
    if (!pdata->vec) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      ret = TEMPDEP_SET_ERROR_NOMEM;
    }

    for (i = 0; i < npow; ++i) {
      SET_P_NEXT(&arg, exact_double, NAN);
      if (!SET_P_PERROR_FINITE(arg, ERROR, "Coefficients must be finite")) {
        ret = TEMPDEP_SET_ERROR_RANGE;
      }

      if (pdata->vec) {
        pdata->vec[i] = arg;
      }
    }
  }

  return ret;
}

static void tempdep_poly_deallocate(union tempdep_property_u *data)
{
  if (data->t_poly.vec) {
    free(data->t_poly.vec);
  }
}

static geom_error tempdep_poly_infomap_gen(geom_info_map *map,
                                           union tempdep_property_u *data,
                                           int domain_is_local)
{
  struct tempdep_poly_data *pdata;
  geom_variant *var;
  geom_error err;
  ptrdiff_t i, npow;

  pdata = &data->t_poly;

  err = GEOM_SUCCESS;
  var = geom_variant_new(&err);
  if (!var) {
    return err;
  }

  npow = pdata->polymax;
  npow = npow + 1;

  geom_variant_set_int(var, pdata->polymax);
  geom_info_map_append(map, var, "Maximum power of polynominal", "", &err);

  if (domain_is_local) {
    tempdep_domain_infomap(&pdata->dom, map, var, &err);
  }

  if (pdata->vec) {
    for (i = 0; i < npow; ++i) {
      int r;
      char *sp;
      const char *csp;
      intmax_t n;

      n = pdata->polymax - i;

      sp = NULL;
      if (n == 0) {
        csp = "Intercept";
      } else if (n > 0) {
        r = jupiter_asprintf(&sp, "Coefficient of T^%" PRIdMAX, n);
        if (r < 0) {
          sp = NULL;
          csp = "Coefficient";
        } else {
          csp = sp;
        }
      } else {
        r = jupiter_asprintf(&sp, "Coefficient of T^(%" PRIdMAX ")", n);
        if (r < 0) {
          sp = NULL;
          csp = "Coefficient";
        } else {
          csp = sp;
        }
      }

      geom_variant_set_double(var, pdata->vec[i]);

      geom_info_map_append(map, var, csp, "", &err);
      free(sp);
    }
  }

  geom_variant_delete(var);
  return err;
}

static const struct tempdep_property_funcs tempdep_poly_funcs = {
  .setter = tempdep_poly_set_csv,
  .deallocator = tempdep_poly_deallocate,
  .infomap_gen = tempdep_poly_infomap_gen,
};

static enum tempdep_set_error
tempdep_poly_l_set_csv(union tempdep_property_u *data, csv_data *csv,
                       const char *fname, csv_row *found_row,
                       csv_column **start,
                       struct tempdep_domain_data *dom)
{
  struct tempdep_poly_l_data *pdata;
  enum tempdep_set_error ret = 0;
  ptrdiff_t npow;
  SET_P_INIT(csv, fname, &found_row, start);

  CSVASSERT(start);

  pdata = &data->t_poly_l;
  pdata->vec = NULL;

  npow = 0;
  SET_P_NEXT(&pdata->polymax, int, 0);
  SET_P_NEXT(&pdata->polymin, int, 0);
  if (pdata->polymax < pdata->polymin) {
    SET_P_PERROR(ERROR, "Maximum power must be greater than minimum");
    ret = TEMPDEP_SET_ERROR_RANGE;
    npow = -1;
  }

  if (npow >= 0) {
    npow = pdata->polymax;
    npow = npow - pdata->polymin + 1;
    if (npow <= 0) {
      SET_P_PERROR(ERROR, "Too wide power range");
      npow = -1;
      ret = TEMPDEP_SET_ERROR_RANGE;
    }
  }

  if (dom) {
    pdata->dom = *dom;
  } else {
    tempdep_domain_set_csv(&pdata->dom, csv, fname, found_row, start, &ret);
  }

  if (npow > 0) {
    ptrdiff_t i;
    double arg;

    pdata->vec = (double *)calloc(sizeof(double), npow);
    if (!pdata->vec) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      ret = TEMPDEP_SET_ERROR_NOMEM;
    }

    for (i = 0; i < npow; ++i) {
      SET_P_NEXT(&arg, exact_double, NAN);
      if (!SET_P_PERROR_FINITE(arg, ERROR, "Coefficients must be finite")) {
        ret = 1;
      }

      if (pdata->vec) {
        pdata->vec[i] = arg;
      }
    }
  }

  return ret;
}

static void tempdep_poly_l_deallocate(union tempdep_property_u *data)
{
  if (data->t_poly_l.vec) {
    free(data->t_poly_l.vec);
  }
}

static geom_error tempdep_poly_l_infomap_gen(geom_info_map *map,
                                             union tempdep_property_u *data,
                                             int domain_is_local)
{
  struct tempdep_poly_l_data *pdata;
  geom_variant *var;
  geom_error err;
  ptrdiff_t i, npow;

  pdata = &data->t_poly_l;

  err = GEOM_SUCCESS;
  var = geom_variant_new(&err);
  if (!var) {
    return err;
  }

  npow = pdata->polymax;
  npow = npow - pdata->polymin + 1;

  geom_variant_set_int(var, pdata->polymax);
  geom_info_map_append(map, var, "Maximum power of polynominal", "", &err);

  geom_variant_set_int(var, pdata->polymin);
  geom_info_map_append(map, var, "Minimum power of polynominal", "", &err);

  if (domain_is_local) {
    tempdep_domain_infomap(&pdata->dom, map, var, &err);
  }

  if (pdata->vec) {
    for (i = 0; i < npow; ++i) {
      int r;
      char *sp;
      const char *csp;
      intmax_t n;

      n = pdata->polymax - i;

      sp = NULL;
      if (n == 0) {
        csp = "Intercept";
      } else if (n > 0) {
        r = jupiter_asprintf(&sp, "Coefficient of T^%" PRIdMAX, n);
        if (r < 0) {
          sp = NULL;
          csp = "Coefficient";
        } else {
          csp = sp;
        }
      } else {
        r = jupiter_asprintf(&sp, "Coefficient of T^(%" PRIdMAX ")", n);
        if (r < 0) {
          sp = NULL;
          csp = "Coefficient";
        } else {
          csp = sp;
        }
      }

      geom_variant_set_double(var, pdata->vec[i]);

      geom_info_map_append(map, var, csp, "", &err);
      free(sp);
    }
  }

  geom_variant_delete(var);
  return err;
}

static const struct tempdep_property_funcs tempdep_poly_l_funcs = {
  .setter = tempdep_poly_l_set_csv,
  .deallocator = tempdep_poly_l_deallocate,
  .infomap_gen = tempdep_poly_l_infomap_gen,
};

static enum tempdep_set_error
tempdep_arrhenius_set_csv(union tempdep_property_u *data, csv_data *csv,
                          const char *fname, csv_row *found_row,
                          csv_column **start,
                          struct tempdep_domain_data *dom)
{
  struct tempdep_arrhenius_data *adata;
  enum tempdep_set_error ret = TEMPDEP_SET_ERROR_SUCCESS;
  SET_P_INIT(csv, fname, &found_row, start);

  CSVASSERT(start);

  adata = &data->t_arrhenius;

  if (dom) {
    adata->dom = *dom;
  } else {
    tempdep_domain_set_csv(&adata->dom, csv, fname, found_row, start, &ret);
  }

  SET_P_NEXT(&adata->coeff_A, exact_double, NAN);
  if (!SET_P_PERROR_FINITE(adata->coeff_A, ERROR,
                           "Coefficient A must be finite")) {
    ret = TEMPDEP_SET_ERROR_RANGE;
  }

  SET_P_NEXT(&adata->coeff_E, exact_double, NAN);
  if (!SET_P_PERROR_FINITE(adata->coeff_E, ERROR,
                           "Coefficient E must be finite")) {
    ret = TEMPDEP_SET_ERROR_RANGE;
  }

  return ret;
}

static geom_error tempdep_arrhenius_infomap_gen(geom_info_map *map,
                                                union tempdep_property_u *data,
                                                int domain_is_local)
{
  struct tempdep_arrhenius_data *adata;
  geom_variant *var;
  geom_error err;

  adata = &data->t_arrhenius;

  err = GEOM_SUCCESS;
  var = geom_variant_new(&err);
  if (!var) {
    return err;
  }

  if (domain_is_local) {
    tempdep_domain_infomap(&adata->dom, map, var, &err);
  }

  geom_variant_set_double(var, adata->coeff_A);
  geom_info_map_append(map, var, "Coefficient A", "I", &err);

  geom_variant_set_double(var, adata->coeff_E);
  geom_info_map_append(map, var, "Coefficient E", "L", &err);

  geom_variant_delete(var);
  return err;
}

static const struct tempdep_property_funcs tempdep_arrhenius_funcs = {
  .setter = tempdep_arrhenius_set_csv,
  .deallocator = NULL,
  .infomap_gen = tempdep_arrhenius_infomap_gen,
};

static enum tempdep_set_error
tempdep_piecewise_set_csv(union tempdep_property_u *data, csv_data *csv,
                          const char *fname, csv_row *found_row,
                          csv_column **start, struct tempdep_domain_data *dom)
{
  enum tempdep_set_error ret = TEMPDEP_SET_ERROR_SUCCESS;
  struct tempdep_piecewise_data *pdata;
  struct tempdep_property prop_dummy;
  SET_P_INIT(csv, fname, &found_row, start);

  pdata = &data->t_piecewise;

  if (dom) {
    /* Internally, it is possible to nest */
    SET_P_PERROR(ERROR, "Piecewise function cannot be nested");
    return TEMPDEP_SET_ERROR_NSPIECE;
  }

  tempdep_property_init(&prop_dummy);

  SET_P_NEXT(&pdata->npieces, int, 0);
  if (pdata->npieces > 0 && pdata->npieces + 1 > 0) { /* +1 is overflow check */
    int i;
    double last_temp, val;

    pdata->domain = (double *)calloc(pdata->npieces + 1, sizeof(double));
    pdata->func = (tempdep_property *)
      calloc(pdata->npieces, sizeof(struct tempdep_property));
    if (!pdata->domain || !pdata->func) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                CSV_ERR_NOMEM, 0, 0, NULL);
      ret = TEMPDEP_SET_ERROR_NOMEM;
    }

    SET_P_NEXT(&last_temp, exact_double, NAN);
    if (!SET_P_PERROR_FINITE(last_temp, ERROR, "Temperature must be finite")) {
      ret = TEMPDEP_SET_ERROR_RANGE;
    }
    if (pdata->domain) {
      pdata->domain[0] = last_temp;
    }

    for (i = 0; i < pdata->npieces; ++i) {
      tempdep_property_type ttype;
      const struct tempdep_property_funcs *func;
      struct tempdep_property *dest;
      struct tempdep_domain_data dom;
      enum tempdep_set_error r;

      SET_P_NEXT(&val, exact_double, NAN);
      if (!SET_P_PERROR_FINITE(val, ERROR, "Temperature must be finite")) {
        ret = TEMPDEP_SET_ERROR_RANGE;
      } else if (last_temp > val) {
        SET_P_PERROR(ERROR, "Temperatures must be in ascending order");
        ret = TEMPDEP_SET_ERROR_RANGE;
      }
      if (pdata->domain) {
        pdata->domain[i + 1] = val;
      }

      dom.minT = last_temp;
      dom.maxT = val;
      last_temp = val;

      SET_P_NEXT(&ttype, tempdep_property_type, TEMPDEP_PROPERTY_INVALID);
      if (ttype == TEMPDEP_PROPERTY_INVALID) {
        ret = TEMPDEP_SET_ERROR_RANGE;
      }
      func = tempdep_get_func(ttype);

      if (pdata->func) {
        dest = &pdata->func[i];
      } else {
        dest = &prop_dummy;
      }

      dest->type = ttype;
      if (func) {
        r = func->setter(&dest->data, csv, fname, found_row, start, &dom);
        if (r != TEMPDEP_SET_ERROR_SUCCESS) {
          ret = r;
        }
      }
      if (dest == &prop_dummy) {
        tempdep_property_clean(&prop_dummy);
      }
    }
  } else {
    pdata->domain = NULL;
    pdata->func = NULL;
    ret = TEMPDEP_SET_ERROR_RANGE;

    if (pdata->npieces <= 0) {
      SET_P_PERROR(ERROR,
                   "Number of pieces for piecewise function must be positive");

    } else if (pdata->npieces + 1 <= 0) {
      SET_P_PERROR(ERROR,
                   "Too big number of pieces specified for piecewise function");
    }
  }

  return ret;
}

static void tempdep_piecewise_deallocate(union tempdep_property_u *data)
{
  if (data->t_piecewise.domain) {
    free(data->t_piecewise.domain);
  }
  if (data->t_piecewise.func) {
    int i;

    CSVASSERT(data->t_piecewise.npieces > 0);

    for (i = 0; i < data->t_piecewise.npieces; i++) {
      tempdep_property_clean(&data->t_piecewise.func[i]);
    }
    free(data->t_piecewise.func);
  }
}

static geom_error tempdep_piecewise_infomap_gen(geom_info_map *map,
                                                union tempdep_property_u *data,
                                                int domain_is_local)
{
  struct tempdep_piecewise_data *pdata;
  geom_variant *var;
  geom_error err;
  int i;

  err = GEOM_SUCCESS;
  var = geom_variant_new(&err);
  if (!var) {
    return err;
  }

  pdata = &data->t_piecewise;

  geom_variant_set_int(var, pdata->npieces);
  geom_info_map_append(map, var, "Number of pieces", "", &err);

  geom_variant_set_double(var, pdata->domain ? pdata->domain[0] : NAN);
  geom_info_map_append(map, var, "Overall minimum domain", "L", &err);

  for (i = 0; i < pdata->npieces; ++i) {
    int r;
    char *sp;
    const char *csp;
    tempdep_property_type ttype;
    union tempdep_property_u *data;
    const struct tempdep_property_funcs *func;

    geom_variant_set_double(var, pdata->domain ? pdata->domain[i + 1] : NAN);

    r = jupiter_asprintf(&sp, "Maximum domain for piece %d", i + 1);
    if (r < 0) {
      sp = NULL;
      csp = "Maximum domain for this piece";
    } else {
      csp = sp;
    }

    geom_info_map_append(map, var, csp, "L", &err);
    free(sp);

    if (pdata->func) {
      ttype = pdata->func[i].type;
      data = &pdata->func[i].data;
    } else {
      ttype = TEMPDEP_PROPERTY_INVALID;
      data = NULL;
    }
    geom_variant_set_enum(var, TEMPDEP_VARTYPE_ENUM_TYPE, ttype);

    r = jupiter_asprintf(&sp, "Funtion for piece %d", i + 1);
    if (r < 0) {
      sp = NULL;
      csp = "Function for this piece";
    } else {
      csp = sp;
    }

    geom_info_map_append(map, var, csp, "", &err);
    free(sp);

    func = NULL;
    if (ttype != TEMPDEP_PROPERTY_INVALID) {
      func = tempdep_get_func(ttype);
    }
    if (func) {
      geom_error e;
      e = func->infomap_gen(map, data, 0);
      if (e != GEOM_SUCCESS) {
        err = e;
      }
    }
  }

  geom_variant_delete(var);
  return err;
}

static const struct tempdep_property_funcs tempdep_piecewise_funcs = {
  .setter = tempdep_piecewise_set_csv,
  .deallocator = tempdep_piecewise_deallocate,
  .infomap_gen = tempdep_piecewise_infomap_gen,
};

static enum tempdep_set_error
tempdep_oxide_set_csv(union tempdep_property_u *data, csv_data *csv,
                      const char *fname, csv_row *found_row,
                      csv_column **start, struct tempdep_domain_data *dom)
{
  SET_P_INIT(csv, fname, &found_row, start);

  /*
   * Basically no parameter exists, but not allowed for piece of piecewise
   * function.
   */
  if (dom) {
    csv_column *found_col;
    found_col = *start;
    SET_P_PERROR(ERROR, "Predefined functions for oxidation model cannot be "
                        "used for a piece of piecewise function");
    return TEMPDEP_SET_ERROR_NSPIECE;
  }
  return TEMPDEP_SET_ERROR_SUCCESS;
}

static const struct tempdep_property_funcs tempdep_oxide_funcs = {
  .setter = tempdep_oxide_set_csv,
  .deallocator = NULL,
  .infomap_gen = NULL,
};

static const struct tempdep_property_funcs *
tempdep_get_func(tempdep_property_type type)
{
  switch (type) {
  case TEMPDEP_PROPERTY_INVALID:
    return NULL;
  case TEMPDEP_PROPERTY_CONST:
    return &tempdep_const_funcs;
  case TEMPDEP_PROPERTY_TABLE:
    return &tempdep_table_funcs;
  case TEMPDEP_PROPERTY_POLY:
    return &tempdep_poly_funcs;
  case TEMPDEP_PROPERTY_POLY_L:
    return &tempdep_poly_l_funcs;
  case TEMPDEP_PROPERTY_ARRHENIUS:
    return &tempdep_arrhenius_funcs;
  case TEMPDEP_PROPERTY_PIECEWISE:
    return &tempdep_piecewise_funcs;

    /* Oxidation only functions does not have extra parameters. */
  case TEMPDEP_PROPERTY_OX_BAKER_JUST:
  case TEMPDEP_PROPERTY_OX_CATHCART_PAWEL:
  case TEMPDEP_PROPERTY_OX_LEISTIKOW_SCHANZ:
  case TEMPDEP_PROPERTY_OX_PRATER_COURTRIGHT:
  case TEMPDEP_PROPERTY_OX_URBANIC_HEIDRICK:
  case TEMPDEP_PROPERTY_OX_RECESSION:
    return &tempdep_oxide_funcs;
  }

  CSVUNREACHABLE();
  return NULL;
}

void tempdep_property_init(tempdep_property *prop)
{
  memset(&prop->data, 0, sizeof(union tempdep_property_u));
  prop->type = TEMPDEP_PROPERTY_INVALID;
}

int tempdep_property_set(tempdep_property *prop, tempdep_property_type type,
                         csv_data *csv, const char *fname, csv_row *found_row,
                         csv_column **start)
{
  enum tempdep_set_error retv;
  const struct tempdep_property_funcs *func;

  CSVASSERT(prop);

  func = tempdep_get_func(type);

  prop->type = type;
  if (func && func->setter) {
    retv = func->setter(&prop->data, csv, fname, found_row, start, NULL);
    if (retv != TEMPDEP_SET_ERROR_SUCCESS) {
      return 1;
    }
    return 0;
  }
  return 0;
}

void tempdep_property_clean(tempdep_property *prop)
{
  const struct tempdep_property_funcs *func;

  CSVASSERT(prop);

  func = tempdep_get_func(prop->type);

  if (func && func->deallocator) {
    func->deallocator(&prop->data);
  }
  prop->type = TEMPDEP_PROPERTY_INVALID;
}

geom_info_map *tempdep_property_create_info_map(tempdep_property *prop,
                                                geom_error *err)
{
  const struct tempdep_property_funcs *func;
  geom_info_map *map;
  geom_error nerr;

  CSVASSERT(prop);

  func = tempdep_get_func(prop->type);

  if (func && func->infomap_gen) {
    map = geom_info_map_new(err);
    if (!map)
      return NULL;

    nerr = func->infomap_gen(map, &prop->data, 1);
    if (nerr != GEOM_SUCCESS) {
      if (err)
        *err = nerr;
      geom_info_map_delete_all(map);
      return NULL;
    }

    return map;

  } else {
    return NULL;
  }
}
