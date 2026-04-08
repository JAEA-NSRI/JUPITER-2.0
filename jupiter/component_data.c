#include "component_data.h"
#include "component_data_defs.h"
#include "csv.h"
#include "csvutil.h"
#include "csvutil_extra.h"
#include "geometry/defs.h"
#include "geometry/list.h"
#include "print_param_keywords.h"
#include "serializer/defs.h"
#include "serializer/msgpackx.h"
#include "serializer/util.h"
#include "struct.h"
#include "os/asprintf.h"

#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

component_data *component_data_new(void)
{
  component_data *p;

  p = (component_data *)malloc(sizeof(component_data));
  if (!p)
    return NULL;

  component_data_init(p);
  return p;
}

void component_data_delete(component_data *p)
{
  geom_list_delete(&p->list);
  if (p->fname)
    free(p->fname);
  if (p->csv)
    freeCSV(p->csv);
  free(p);
}

void component_data_delete_all(component_data *head)
{
  struct geom_list *lp, *ln, *lh;
  lh = &head->list;
  geom_list_foreach_safe (lp, ln, lh) {
    component_data_delete(component_data_entry(lp));
  }
}

component_data *component_data_find(component_data *head,
                                    component_data_find_func *func, void *arg)
{
  struct geom_list *lp, *lh;
  lh = &head->list;
  geom_list_foreach (lp, lh) {
    component_data *d;
    d = component_data_entry(lp);
    if (func(d, arg))
      return d;
  }
  return NULL;
}

static int component_data_find_jupiter_id(const component_data *d, void *arg)
{
  return d->jupiter_id == *(int *)arg;
}

component_data *component_data_find_by_jupiter_id(component_data *head,
                                                  int jupiter_id)
{
  return component_data_find(head, component_data_find_jupiter_id, &jupiter_id);
}

static int component_data_find_comp_index(const component_data *d, void *arg)
{
  return d->comp_index == *(int *)arg;
}

component_data *component_data_find_by_comp_index(component_data *head,
                                                  int comp_index)
{
  return component_data_find(head, component_data_find_comp_index, &comp_index);
}

static int component_data_update_sort_comp_id(component_data *a,
                                              component_data *b)
{
  if (a->generated && b->generated) {
    if (a->comp_index > b->comp_index)
      return -1;
    if (a->comp_index < b->comp_index)
      return 1;
    return 0;
  }
  if (a->generated)
    return -1;
  if (b->generated)
    return 1;

  if (a->jupiter_id > b->jupiter_id)
    return -1;
  if (b->jupiter_id < a->jupiter_id)
    return 1;
  return 0;
}

static int component_data_update_sort_comp(struct geom_list *la,
                                           struct geom_list *lb)
{
  int idnva, idnvb, idm1a, idm1b, idsla, idslb, idga, idgb;
  component_data *a, *b;
  a = component_data_entry(la);
  b = component_data_entry(lb);

  idnva = !component_phases_is_valid(a->phases);
  idnvb = !component_phases_is_valid(b->phases);

  if (idnva && idnvb)
    return 0; /* not comparable */
  if (idnva)
    return 1;
  if (idnvb)
    return -1;

  idm1a = a->jupiter_id == -1;
  idm1b = b->jupiter_id == -1;

  if (idm1a && idm1b)
    return 0;
  if (idm1a)
    return 1;
  if (idm1b)
    return -1;

  idga = component_phases_is_gas_only(a->phases);
  idgb = component_phases_is_gas_only(b->phases);

  if (idga && idgb)
    return component_data_update_sort_comp_id(a, b);
  if (idga)
    return -1;
  if (idgb)
    return 1;

  idsla = component_phases_has_solid_or_liquid(a->phases);
  idslb = component_phases_has_solid_or_liquid(b->phases);

  if (idsla && idslb)
    return component_data_update_sort_comp_id(a, b);
  if (idsla)
    return 1;
  if (idslb)
    return -1;

  /* Both invalid entry */
  return 0;
}

void component_data_update_index(component_data *head, domain *cdo, int *stat)
{
  struct geom_list *lp, *lh;
  int i;
  component_data *dn;

  /* set index in list order to generated entries */
  lh = &head->list;
  i = 0;
  geom_list_foreach (lp, lh) {
    component_data *d;
    d = component_data_entry(lp);

    if (d->generated)
      d->comp_index = i++;
  }

  geom_list_sort(lh, component_data_update_sort_comp);

  i = 0;
  dn = NULL;
  cdo->NBaseComponent = -1;
  geom_list_foreach (lp, lh) {
    component_data *d;
    d = component_data_entry(lp);

    if (!component_phases_is_valid(d->phases))
      break;

    if (d->jupiter_id == -1) {
      CSVASSERT(i == 0);
      if (dn) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Multiple components have ID -1");
        if (stat)
          *stat = ON;
      }
      if (!component_phases_is_gas_only(d->phases)) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "ID -1 must be gas-only component");
        if (stat)
          *stat = ON;
      }

      d->comp_index = -1;
    } else {
      if (!dn) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "No entry found for ID -1");
        if (stat)
          *stat = ON;
      }

      if ((!dn || i == 0 || component_phases_has_liquid(dn->phases)) &&
          component_phases_is_gas_only(d->phases)) {
        cdo->NBaseComponent = i;
      }
      d->comp_index = i++;
    }
    dn = d;
  }

  cdo->NumberOfComponent = i;
  if (cdo->NumberOfComponent <= 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Least one component is required");
    if (stat)
      *stat = ON;
  }

  if (!component_phases_is_gas_only(dn->phases)) {
    cdo->NBaseComponent = cdo->NumberOfComponent;
    cdo->NGasComponent = 0;
  } else {
    if (cdo->NBaseComponent < 0)
      cdo->NBaseComponent = 0;
    cdo->NGasComponent = cdo->NumberOfComponent - cdo->NBaseComponent;
  }

  if (cdo->NBaseComponent <= 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Least one solid-liquid component is required");
    if (stat)
      *stat = ON;
  }
}

#define COMP_DATA_TITLE "comp_data"
#define COMP_DATA_VERSION 1

int component_data_read(const char *file, component_data *head,
                        struct component_data_metadata *metaout)
{
  FILE *fp;
  msgpackx_data *data;
  msgpackx_error err;
  ptrdiff_t eloc;

  errno = 0;
  fp = fopen(file, "rb");
  if (!fp) {
    csvperror(file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_FOPEN, errno, 0, NULL);
    return 2;
  }

  data = msgpackx_data_read(fp, &err, &eloc);
  if (data) {
    err = component_data_parse(data, head, metaout);
    msgpackx_data_delete(data);
  }
  if (err != MSGPACKX_SUCCESS) {
    csvperror(file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, errno, err,
              NULL);
  }
  fclose(fp);
  if (err == MSGPACKX_SUCCESS)
    return 0;
  return 1;
}

static int component_data_ovf_check(intmax_t ival, msgpackx_error *err)
{
  if (ival >= INT_MIN && ival <= INT_MAX)
    return ival;
  if (err)
    *err = MSGPACKX_ERR_RANGE;
  return 0;
}

static msgpackx_error component_data_get_int(int *outp,
                                             msgpackx_map_node *mhead,
                                             const char *keystr,
                                             ptrdiff_t keylen)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  intmax_t ival;

  ival = msgpackx_map_skey_get_int(mhead, keystr, keylen, &err);
  if (err != MSGPACKX_SUCCESS) {
    *outp = component_data_ovf_check(ival, NULL);
    return err;
  }

  *outp = component_data_ovf_check(ival, &err);
  return err;
}

static msgpackx_error component_data_aget_int(int *outp,
                                              msgpackx_array_node *anode)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  intmax_t ival;

  ival = msgpackx_array_node_get_int(anode, &err);
  if (err != MSGPACKX_SUCCESS) {
    *outp = component_data_ovf_check(ival, NULL);
    return err;
  }

  *outp = component_data_ovf_check(ival, &err);
  return err;
}

static msgpackx_error component_data_aget_vec(int *nx, int *ny, int *nz,
                                              msgpackx_array_node *ahead)
{
  msgpackx_error err;
  msgpackx_array_node *anode;

  anode = msgpackx_array_node_next(ahead);
  if (anode == ahead)
    return MSGPACKX_ERR_INDEX;

  err = component_data_aget_int(nx, anode);
  if (err != MSGPACKX_SUCCESS)
    return err;

  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    return MSGPACKX_ERR_INDEX;

  err = component_data_aget_int(ny, anode);
  if (err != MSGPACKX_SUCCESS)
    return err;

  anode = msgpackx_array_node_next(anode);
  if (anode == ahead)
    return MSGPACKX_ERR_INDEX;

  err = component_data_aget_int(nz, anode);
  return err;
}

static char *component_data_strndup(const char *p, ptrdiff_t len,
                                    msgpackx_error *err)
{
  char *astr;

  if (len < 0 || len >= PTRDIFF_MAX) {
    if (err)
      *err = MSGPACKX_ERR_RANGE;
    return NULL;
  }

  astr = jupiter_strndup(p, len);
  if (!astr) {
    if (err)
      *err = MSGPACKX_ERR_NOMEM;
    return NULL;
  }
  return astr;
}

msgpackx_error component_data_parse(msgpackx_data *data, component_data *head,
                                    struct component_data_metadata *metaout)
{
  intmax_t version;
  msgpackx_array_node *anode, *ahead;
  msgpackx_map_node *thead, *metahead;
  msgpackx_error err = MSGPACKX_SUCCESS;

  anode = msgpackx_read_header_data(data, COMP_DATA_TITLE, NULL, &err);
  if (err != MSGPACKX_SUCCESS)
    return err;

  thead = msgpackx_array_node_get_map(anode, &err);
  if (err != MSGPACKX_SUCCESS)
    return err;

  version = msgpackx_map_skey_get_int(thead, MCSTR("v"), &err);
  if (err != MSGPACKX_SUCCESS)
    return err;

  if (version != COMP_DATA_VERSION) {
    csvperrorf(
      __FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
      "Component data file version is not expected value %d, got " PRIdMAX "",
      COMP_DATA_VERSION, version);
    return MSGPACKX_ERR_RANGE;
  }

  if (metaout) {
    metahead = msgpackx_map_skey_get_map(thead, MCSTR("m"), &err);
    if (err != MSGPACKX_SUCCESS) {
      if (err != MSGPACKX_ERR_KEYNOTFOUND)
        return err;
      err = MSGPACKX_SUCCESS;
      metahead = NULL;
    }
    if (metahead) {
      msgpackx_array_node *vechead;

      err = component_data_get_int(&metaout->NumberOfComponent, metahead,
                                   MCSTR("NumberOfComponent"));
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_get_int(&metaout->NBaseComponent, metahead,
                                   MCSTR("NBaseComponent"));
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_get_int(&metaout->NGasComponent, metahead,
                                   MCSTR("NGasComponent"));
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_get_int(&metaout->NIComponent, metahead,
                                   MCSTR("NIComponent"));
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_get_int(&metaout->NIBaseCopmonent, metahead,
                                   MCSTR("NIBaseComponent"));
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_get_int(&metaout->NIGasCopmonent, metahead,
                                   MCSTR("NIGasComponent"));
      if (err != MSGPACKX_SUCCESS)
        return err;

      vechead = msgpackx_map_skey_get_ary(metahead, MCSTR("gn"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_aget_vec(&metaout->gnx, &metaout->gny, &metaout->gnz,
                                    vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;

      vechead = msgpackx_map_skey_get_ary(metahead, MCSTR("n"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_aget_vec(&metaout->nx, &metaout->ny, &metaout->nz,
                                    vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;

      vechead = msgpackx_map_skey_get_ary(metahead, MCSTR("npe"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_aget_vec(&metaout->pex, &metaout->pey, &metaout->pez,
                                    vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;

      metaout->solute_diff =
        msgpackx_map_skey_get_bool(metahead, MCSTR("solute_diff"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      metaout->solute_diff = metaout->solute_diff ? ON : OFF;

      metaout->qgeom =
        msgpackx_map_skey_get_bool(metahead, MCSTR("qgeom"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      metaout->bnd_norm_u =
        msgpackx_map_skey_get_bool(metahead, MCSTR("bnd_norm_u"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      metaout->bnd_norm_v =
        msgpackx_map_skey_get_bool(metahead, MCSTR("bnd_norm_v"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      metaout->bnd_norm_w =
        msgpackx_map_skey_get_bool(metahead, MCSTR("bnd_norm_w"), &err);
      if (err != MSGPACKX_SUCCESS)
        return err;

      if (err != MSGPACKX_SUCCESS)
        return err;

      {
        char *aomode;
        const char *omode;
        ptrdiff_t len = 0;

        metaout->output_mode = BINARY_OUTPUT_INVALID;
        omode =
          msgpackx_map_skey_get_str(metahead, MCSTR("output_mode"), &len, &err);

        if (err != MSGPACKX_SUCCESS)
          return err;

        aomode = component_data_strndup(omode, len, &err);
        if (err != MSGPACKX_SUCCESS)
          return err;

        metaout->output_mode = str_to_binary_output_mode(aomode);
        free(aomode);
      }
    } else {
      memset(metaout, 0, sizeof(struct component_data_metadata));
      metaout->NumberOfComponent = JUPITER_ID_INVALID;
    }
  }

  ahead = msgpackx_map_skey_get_ary(thead, MCSTR("d"), &err);
  if (err != MSGPACKX_SUCCESS)
    return err;

  anode = msgpackx_array_node_next(ahead);
  for (; anode != ahead; anode = msgpackx_array_node_next(anode)) {
    msgpackx_map_node *mhead;
    component_data *d;
    d = component_data_new();
    if (!d) {
      return MSGPACKX_ERR_NOMEM;
    }

    do {
      msgpackx_array_node *phases, *iter;
      msgpackx_map_node *indices;

      mhead = msgpackx_array_node_get_map(anode, &err);
      if (err != MSGPACKX_SUCCESS)
        break;

      err = component_data_get_int(&d->jupiter_id, mhead, MCSTR("id"));
      if (err != MSGPACKX_SUCCESS)
        break;

      d->generated =
        msgpackx_map_skey_get_bool(mhead, MCSTR("generated"), &err);
      if (err != MSGPACKX_SUCCESS)
        break;

      indices = msgpackx_map_skey_get_map(mhead, MCSTR("indices"), &err);
      if (err != MSGPACKX_SUCCESS)
        break;

      err = component_data_get_int(&d->comp_index, indices, MCSTR("comp"));
      if (err != MSGPACKX_SUCCESS)
        break;

      err =
        component_data_get_int(&d->phase_comps_index, indices, MCSTR("phase"));
      if (err != MSGPACKX_SUCCESS)
        break;

      err = component_data_get_int(&d->mass_source_g_index, indices,
                                   MCSTR("mass_source_g"));
      if (err != MSGPACKX_SUCCESS)
        break;

      err = component_data_get_int(&d->lpt_mass_fraction_index, indices,
                                   MCSTR("lpt_mass_fraction"));
      if (err != MSGPACKX_SUCCESS)
        break;

      phases = msgpackx_map_skey_get_ary(mhead, MCSTR("phases"), &err);
      if (err != MSGPACKX_SUCCESS)
        break;

      d->phases = component_phases_none();
      iter = msgpackx_array_node_next(phases);
      for (; iter != phases; iter = msgpackx_array_node_next(iter)) {
        const char *cstr;
        ptrdiff_t len;
        char *astr;

        cstr = msgpackx_array_node_get_str(iter, &len, &err);
        if (err != MSGPACKX_SUCCESS)
          break;

        astr = component_data_strndup(cstr, len, &err);
        if (err != MSGPACKX_SUCCESS)
          break;

        do {
          component_phase_name phase = str_to_component_phase(astr);
          if (phase == COMPONENT_PHASE_MAX) {
            err = MSGPACKX_ERR_MSG_TYPE;
            break;
          }

          component_phases_set(&d->phases, phase, 1);
        } while (0);
        free(astr);
        if (err != MSGPACKX_SUCCESS)
          break;
      }
      if (err != MSGPACKX_SUCCESS)
        break;
    } while (0);
    if (err != MSGPACKX_SUCCESS) {
      component_data_delete(d);
      return err;
    }

    geom_list_insert_prev(&head->list, &d->list);
  }

  return MSGPACKX_SUCCESS;
}

static msgpackx_error component_data_aset_vec(int nx, int ny, int nz,
                                              msgpackx_array_node *ahead)
{
  msgpackx_error err;

  CSVASSERT(msgpackx_array_node_next(ahead) == ahead); /* assume empty */

  err = msgpackx_array_append_int(ahead, nx);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_append_int(ahead, ny);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_append_int(ahead, nz);
  return err;
}

msgpackx_error component_data_pack(component_data *head,
                                   struct component_data_metadata *metadata,
                                   msgpackx_data *data)
{
  struct geom_list *lp, *lh;
  msgpackx_array_node *anode, *ahead;
  msgpackx_map_node *thead, *metahead;
  msgpackx_error err = MSGPACKX_SUCCESS;

  anode = msgpackx_make_header_data(data, COMP_DATA_TITLE, NULL, &err);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_insert_prev_map(anode, &thead);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_map_skey_set_int(thead, MCSTR("v"), COMP_DATA_VERSION);
  if (err != MSGPACKX_SUCCESS)
    return err;

  if (metadata) {
    err = msgpackx_map_skey_set_map(thead, MCSTR("m"), &metahead);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(metahead, MCSTR("NumberOfComponent"),
                                    metadata->NumberOfComponent);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(metahead, MCSTR("NBaseComponent"),
                                    metadata->NBaseComponent);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(metahead, MCSTR("NGasComponent"),
                                    metadata->NGasComponent);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(metahead, MCSTR("NIComponent"),
                                    metadata->NIComponent);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(metahead, MCSTR("NIBaseComponent"),
                                    metadata->NIBaseCopmonent);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(metahead, MCSTR("NIGasComponent"),
                                    metadata->NIGasCopmonent);
    if (err != MSGPACKX_SUCCESS)
      return err;

    {
      msgpackx_array_node *vechead;
      err = msgpackx_map_skey_set_ary(metahead, MCSTR("gn"), &vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_aset_vec(metadata->gnx, metadata->gny, metadata->gnz,
                                    vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;
    }
    {
      msgpackx_array_node *vechead;
      err = msgpackx_map_skey_set_ary(metahead, MCSTR("n"), &vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_aset_vec(metadata->nx, metadata->ny, metadata->nz,
                                    vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;
    }
    {
      msgpackx_array_node *vechead;
      err = msgpackx_map_skey_set_ary(metahead, MCSTR("npe"), &vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;

      err = component_data_aset_vec(metadata->pex, metadata->pey, metadata->pez,
                                    vechead);
      if (err != MSGPACKX_SUCCESS)
        return err;
    }

    err = msgpackx_map_skey_set_bool(metahead, MCSTR("solute_diff"),
                                     !!metadata->solute_diff);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err =
      msgpackx_map_skey_set_bool(metahead, MCSTR("qgeom"), !!metadata->qgeom);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_bool(metahead, MCSTR("bnd_norm_u"),
                                     !!metadata->bnd_norm_u);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_bool(metahead, MCSTR("bnd_norm_v"),
                                     !!metadata->bnd_norm_v);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_bool(metahead, MCSTR("bnd_norm_w"),
                                     !!metadata->bnd_norm_w);
    if (err != MSGPACKX_SUCCESS)
      return err;

    {
      const char *omode;
      omode = PP_binary_output_mode_value_format_v(metadata->output_mode);
      if (omode) {
        err = msgpackx_map_skey_set_str(metahead, MCSTR("output_mode"), omode,
                                        strlen(omode));
      } else {
        err =
          msgpackx_map_skey_set_str(metahead, MCSTR("output_mode"), MCSTR(""));
      }
      if (err != MSGPACKX_SUCCESS)
        return err;
    }
  }

  err = msgpackx_map_skey_set_ary(thead, MCSTR("d"), &ahead);
  if (err != MSGPACKX_SUCCESS)
    return err;

  lh = &head->list;
  geom_list_foreach (lp, lh) {
    component_data *p;
    msgpackx_map_node *mhead, *mindices;
    msgpackx_array_node *phases_ahead;

    p = component_data_entry(lp);

    err = msgpackx_array_append_map(ahead, &mhead);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(mhead, MCSTR("id"), p->jupiter_id);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_bool(mhead, MCSTR("generated"), !!p->generated);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_map(mhead, MCSTR("indices"), &mindices);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(mindices, MCSTR("comp"), p->comp_index);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err =
      msgpackx_map_skey_set_int(mindices, MCSTR("phase"), p->phase_comps_index);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(mindices, MCSTR("mass_source_g"),
                                    p->mass_source_g_index);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_int(mindices, MCSTR("lpt_mass_fraction"),
                                    p->lpt_mass_fraction_index);
    if (err != MSGPACKX_SUCCESS)
      return err;

    err = msgpackx_map_skey_set_ary(mhead, MCSTR("phases"), &phases_ahead);
    if (err != MSGPACKX_SUCCESS)
      return err;

    for (int ip = 0; ip < COMPONENT_PHASE_MAX; ++ip) {
      if (component_phases_get(p->phases, ip)) {
        const char *name = PP_component_phase_value_format_v(ip);
        err = msgpackx_array_append_str(phases_ahead, name, strlen(name));
        if (err != MSGPACKX_SUCCESS)
          return err;
      }
    }
  }

  return MSGPACKX_SUCCESS;
}

int component_data_write(const char *file, component_data *head,
                         struct component_data_metadata *metadata)
{
  FILE *fp;
  msgpackx_data *d;
  msgpackx_error err;
  ptrdiff_t r;

  d = msgpackx_data_new();
  if (!d) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }

  errno = 0;
  err = component_data_pack(head, metadata, d);
  if (err != MSGPACKX_SUCCESS) {
    csvperror(file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, errno, err,
              NULL);
    return 1;
  }

  errno = 0;
  fp = fopen(file, "wb");
  if (!fp) {
    csvperror(file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_FOPEN, errno, 0, NULL);
    msgpackx_data_delete(d);
    return 2;
  }

  r = msgpackx_data_write(d, fp);

  fclose(fp);
  msgpackx_data_delete(d);

  if (r < 0) {
    if (errno != 0) {
      csvperror(file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);
    } else {
      csvperrorf(file, 0, 0, CSV_EL_ERROR, NULL, "Failed to write");
    }
    return 1;
  }
  if (err != MSGPACKX_SUCCESS) {
    csvperror(file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, errno, err,
              NULL);
    return 1;
  }
  return 0;
}

void component_data_metadata_get(struct component_data_metadata *outp,
                                 domain *cdo, flags *flg, variable *val,
                                 binary_output_mode output_mode)
{
  outp->NumberOfComponent = cdo->NumberOfComponent;
  outp->NBaseComponent = cdo->NBaseComponent;
  outp->NGasComponent = cdo->NGasComponent;
  outp->NIComponent = cdo->NIComponent;
  outp->NIBaseCopmonent = cdo->NIBaseComponent;
  outp->NIGasCopmonent = cdo->NIGasComponent;
  outp->gnx = cdo->gnx;
  outp->gny = cdo->gny;
  outp->gnz = cdo->gnz;
  outp->nx = cdo->nx;
  outp->ny = cdo->ny;
  outp->nz = cdo->nz;
  outp->pex = flg->pex;
  outp->pey = flg->pey;
  outp->pez = flg->pez;
  outp->solute_diff = flg->solute_diff;
  outp->qgeom = !!val->qgeom;
  outp->bnd_norm_u = !!val->bnd_norm_u;
  outp->bnd_norm_v = !!val->bnd_norm_v;
  outp->bnd_norm_w = !!val->bnd_norm_w;
  outp->output_mode = output_mode;
}
