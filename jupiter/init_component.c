#include "init_component.h"
#include "csvutil.h"
#include "struct.h"
#include "func.h"

void init_component_for_initial(init_component *dest, parameter *prm)
{
  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);

  domain *cdo = prm->cdo;
  flags *flg = prm->flg;
  init_component c = init_component_all();

  if (flg->lpt_calc != ON)
    init_component_unset(&c, INIT_COMPONENT_LPT_PEWALL_N);
  init_component_band(dest, dest, &c);
}

void init_component_for_update_control(init_component *dest, parameter *prm)
{
  init_component_for_initial(dest, prm);
}
