#include <limits.h>

#include "test-util.h"

#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/bitarray.h>
#include <jupiter/geometry/geom_static_assert.h>

enum StarIds
{
  Sirius,
  Canopus,
  Arcturus,
  Rigil,
  Vega,
  Capella,
  Rigel,
  Procyon,
  Achernar,
  Betelgeuse,
  Hadar,
  Altair,
  Aldebaran,
  Spica,
  Antares,
  Pollux,
  Fomalhaut,
  Deneb,
  Mimosa,
  Acrux,
  Toliman,
  Regulus,
  Adhara,
  Gacrux,
  Shaula,
  Bellatrix,
  Elnath,
  Miaplacidus,
  Alnilam,
  Alnair,
  Alnitak,
  Alioth,
  Kaus,
  Mirfak,
  Dubhe,
  Wezen,
  Alkaid,
  Avior,
  Sargas,
  Menkalinan,
  Atria,
  Alhena,
  Peacock,
  Polaris,
  Castor,
  Mirzam,
  Alphard,
  Alsephina,
  Hamal,
  Diphda,
  Nunki,
  Menkent,
  Alpheratz,
  Mirach,
  Saiph,
  Kochab,
  Rasalhague,
  Algol,
  Almach,
  Tiaki,
  Denebola,
  Aspidiske,
  Naos,
  Alphecca,
  Mizar,
  Sadr,
  Suhail,
  Schedar,
  Eltanin,
  Mintaka,
  Caph,

  N_BITS,
};

static geom_bitarray_n(stars, N_BITS);

GEOM_STATIC_ASSERT(sizeof(stars) * CHAR_BIT >= N_BITS, "");
GEOM_STATIC_ASSERT(sizeof(stars) / sizeof(stars[0]) ==
                     GEOM_BITARRAY_N_WORD(N_BITS),
                   "");

int bitarray_test(void)
{
  int ecnt;

  geom_bitarray_n(o_stars, N_BITS);
  geom_bitarray_n(only_one, 1);

  ecnt = 0;
  fprintf(stderr, "..... N_WORD_BITS: %zd\n", GEOM_BITARRAY_WORD_BITS);
  fprintf(stderr, "..... N_BITS: %d\n", N_BITS);
  fprintf(stderr, "..... N_WORDS: %zd\n", GEOM_BITARRAY_N_WORD(N_BITS));
  fprintf(stderr, "..... sizeof(stars): %zd\n", sizeof(stars));
  if (test_compare_f(sizeof(stars) / sizeof(stars[0]),
                     GEOM_BITARRAY_N_WORD(N_BITS), "Wrowg sized bitarray"))
    return 1;

  geom_bitarray_element_setall(stars, N_BITS, 0);
  if (test_compare_f(geom_bitarray_element_getany(stars, N_BITS), 0,
                     "Must be all bits off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(stars, Tiaki), 0,
                     "Tiaki must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(stars, Almach), 0,
                     "Almach must be off"))
    ecnt++;

  geom_bitarray_element_set(stars, Almach, 1);
  if (test_compare_f(geom_bitarray_element_get(stars, Almach), 1,
                     "Almach must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(stars, Tiaki), 0,
                     "Tiaki must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(stars, Algol), 0,
                     "Algol must be off"))
    ecnt++;

  geom_bitarray_element_set(stars, Algol, 1);
  if (test_compare_f(geom_bitarray_element_get(stars, Algol), 1,
                     "Algol must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(stars, Almach), 1,
                     "Almach must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(stars, Tiaki), 0,
                     "Tiaki must be off"))
    ecnt++;

  geom_bitarray_element_set(stars, Spica, 0);
  if (test_compare_f(geom_bitarray_element_get(stars, Spica), 0,
                     "Spica must be off"))
    ecnt++;

  geom_bitarray_element_set(stars, Almach, 0);
  if (test_compare_f(geom_bitarray_element_get(stars, Almach), 0,
                     "Almach must be off"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 0);
  geom_bitarray_element_set(stars, Spica, 1);
  if (test_compare_f(geom_bitarray_element_getany(stars, N_BITS), 1,
                     "Must be some bits on"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 0);
  geom_bitarray_element_set(stars, Sargas, 1);
  if (test_compare_f(geom_bitarray_element_getany(stars, N_BITS), 1,
                     "Must be some bits on"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 0);
  geom_bitarray_element_set(stars, Caph, 1);
  if (test_compare_f(geom_bitarray_element_getany(stars, N_BITS), 1,
                     "Must be some bits on"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 1);
  if (test_compare_f(geom_bitarray_element_getall(stars, N_BITS), 1,
                     "Must be all bits on"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 1);
  geom_bitarray_element_set(stars, Aldebaran, 0);
  if (test_compare_f(geom_bitarray_element_getall(stars, N_BITS), 0,
                     "Must be some bits off"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 1);
  geom_bitarray_element_set(stars, Sargas, 0);
  if (test_compare_f(geom_bitarray_element_getall(stars, N_BITS), 0,
                     "Must be some bits off"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 1);
  geom_bitarray_element_set(stars, Caph, 0);
  if (test_compare_f(geom_bitarray_element_getall(stars, N_BITS), 0,
                     "Must be some bits off"))
    ecnt++;

  geom_bitarray_element_setall(stars, N_BITS, 0);
  geom_bitarray_element_set(stars, Aldebaran, 1);
  geom_bitarray_element_set(stars, Spica, 1);
  geom_bitarray_element_set(stars, Rigil, 1);
  geom_bitarray_element_set(stars, Regulus, 1);
  geom_bitarray_element_set(stars, Alphecca, 1);
  geom_bitarray_element_set(stars, Caph, 1);
  geom_bitarray_element_setall(o_stars, N_BITS, 0);
  geom_bitarray_element_copy(o_stars, stars, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Aldebaran), 1,
                     "Aldebaran must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Spica), 1,
                     "Spica must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Rigil), 1,
                     "Rigil must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Alphecca), 1,
                     "Alphecca must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Caph), 1,
                     "Caph must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 0,
                     "Vega must be off"))
    ecnt++;

  geom_bitarray_element_set(o_stars, Aldebaran, 0);
  geom_bitarray_element_set(o_stars, Spica, 0);
  geom_bitarray_element_set(o_stars, Rigil, 0);
  geom_bitarray_element_set(o_stars, Regulus, 0);
  geom_bitarray_element_set(o_stars, Alphecca, 0);
  geom_bitarray_element_set(o_stars, Caph, 0);

  if (test_compare_f(geom_bitarray_element_getany(o_stars, N_BITS), 0,
                     "Must be all bits off"))
    ecnt++;

  geom_bitarray_element_setall(only_one, 1, 0);
  if (test_compare_f(geom_bitarray_element_getany(only_one, 1), 0,
                     "Must be all bits off"))
    ecnt++;

  geom_bitarray_element_set(only_one, 0, 1);
  if (test_compare_f(geom_bitarray_element_getall(only_one, 1), 1,
                     "Must be all bits on"))
    ecnt++;

  geom_bitarray_element_setall(only_one, 1, 1);
  if (test_compare_f(geom_bitarray_element_getall(only_one, 1), 1,
                     "Must be all bits on"))
    ecnt++;

  geom_bitarray_element_set(only_one, 0, 0);
  if (test_compare_f(geom_bitarray_element_getany(only_one, 1), 0,
                     "Must be all bits off"))
    ecnt++;

  geom_bitarray_n(o_stars1, N_BITS);
  geom_bitarray_n(o_stars2, N_BITS);
  geom_bitarray_element_setall(o_stars1, N_BITS, 0);
  geom_bitarray_element_setall(o_stars2, N_BITS, 0);
  geom_bitarray_element_setall(o_stars, N_BITS, 0);
  geom_bitarray_element_set(o_stars1, Deneb, 1);
  geom_bitarray_element_set(o_stars1, Vega, 1);
  geom_bitarray_element_set(o_stars2, Deneb, 1);
  geom_bitarray_element_set(o_stars2, Sirius, 1);

  geom_bitarray_element_bor(o_stars, o_stars1, o_stars2, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 1,
                     "Vega must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 1,
                     "Sirius must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 0,
                     "Canopus must be off"))
    ecnt++;

  geom_bitarray_element_bor(o_stars, o_stars1, o_stars1, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 1,
                     "Vega must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 0,
                     "Sirius must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 0,
                     "Canopus must be off"))
    ecnt++;

  geom_bitarray_element_set(o_stars, Canopus, 1);
  geom_bitarray_element_bor(o_stars, o_stars, o_stars, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 1,
                     "Vega must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 0,
                     "Sirius must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 1,
                     "Canopus must be on"))
    ecnt++;

  geom_bitarray_element_band(o_stars, o_stars1, o_stars2, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 0,
                     "Vega must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 0,
                     "Sirius must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 0,
                     "Canopus must be off"))
    ecnt++;

  geom_bitarray_element_band(o_stars, o_stars2, o_stars2, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 0,
                     "Vega must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 1,
                     "Sirius must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 0,
                     "Canopus must be off"))
    ecnt++;

  geom_bitarray_element_band(o_stars, o_stars2, o_stars2, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 0,
                     "Vega must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 1,
                     "Sirius must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 0,
                     "Canopus must be off"))
    ecnt++;

  geom_bitarray_element_set(o_stars, Canopus, 1);
  geom_bitarray_element_band(o_stars, o_stars, o_stars, N_BITS);
  if (test_compare_f(geom_bitarray_element_get(o_stars, Deneb), 1,
                     "Deneb must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Vega), 0,
                     "Vega must be off"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Sirius), 1,
                     "Sirius must be on"))
    ecnt++;

  if (test_compare_f(geom_bitarray_element_get(o_stars, Canopus), 1,
                     "Canopus must be off"))
    ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 0);
  if (test_compare_f(geom_bitarray_element_eql(o_stars, o_stars1, N_BITS), 0,
                     "Not equal"))
      ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 1);
  if (test_compare_f(geom_bitarray_element_eql(o_stars, o_stars1, N_BITS), 1,
                     "Equal"))
    ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 1);
  geom_bitarray_element_set(o_stars1, Sirius, 0);
  if (test_compare_f(geom_bitarray_element_eql(o_stars, o_stars1, N_BITS), 0,
                     "Not Equal"))
    ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 0);
  for (int i = 0; i < N_BITS; ++i)
    geom_bitarray_element_set(o_stars1, i, 1);
  if (test_compare_f(geom_bitarray_element_eql(o_stars, o_stars1, N_BITS), 1,
                     "Equal"))
    ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 0);
  if (test_compare_f(geom_bitarray_element_neq(o_stars, o_stars1, N_BITS), 1,
                     "Not equal"))
      ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 1);
  if (test_compare_f(geom_bitarray_element_neq(o_stars, o_stars1, N_BITS), 0,
                     "Equal"))
    ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 1);
  geom_bitarray_element_set(o_stars1, Sirius, 0);
  if (test_compare_f(geom_bitarray_element_neq(o_stars, o_stars1, N_BITS), 1,
                     "Not Equal"))
    ecnt++;

  geom_bitarray_element_setall(o_stars, N_BITS, 1);
  geom_bitarray_element_setall(o_stars1, N_BITS, 0);
  for (int i = 0; i < N_BITS; ++i)
    geom_bitarray_element_set(o_stars1, i, 1);
  if (test_compare_f(geom_bitarray_element_neq(o_stars, o_stars1, N_BITS), 0,
                     "Equal"))
    ecnt++;


  return ecnt != 0;
}
