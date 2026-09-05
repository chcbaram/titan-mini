#include "module.h"


typedef struct
{
  int32_t         count;
  const module_t *p_module;
} module_info_t;


#if CLI_USE(HW_MODULE)
static void cliModule(cli_args_t *args);
#endif

static bool moduleBegin(void);

static module_info_t info;

extern uint32_t _smodule;
extern uint32_t _emodule;




bool moduleInit(void)
{
  bool ret;

  info.count    = ((int)&_emodule - (int)&_smodule) / sizeof(module_t);
  info.p_module = (const module_t *)&_smodule;

  logPrintf("[  ] moduleInit()\r\n");
  logPrintf("       count : %d\r\n", (int)info.count);

  ret = moduleBegin();

#if CLI_USE(HW_MODULE)
  cliAdd("module", cliModule);
#endif

  return ret;
}

bool moduleBegin(void)
{
  bool ret = true;

  for (int pri = MODULE_PRI_HIGH; pri < MODULE_PRI_MAX; pri++)
  {
    for (int i = 0; i < info.count; i++)
    {
      const module_t *p_mod = &info.p_module[i];

      if (p_mod->priority < MODULE_PRI_HIGH || p_mod->priority >= MODULE_PRI_MAX)
      {
        //-- 우선순위가 범위 밖이면 영원히 안 불린다. 조용히 넘기지 않는다.
        //
        if (pri == MODULE_PRI_HIGH)
        {
          logPrintf("       %s priority %d 범위 밖\r\n", p_mod->name, (int)p_mod->priority);
          ret = false;
        }
        continue;
      }

      if (p_mod->priority == pri && p_mod->init != NULL)
      {
        bool mod_ret = p_mod->init();

        ret &= mod_ret;
        logPrintf("       %-16s %s\r\n", p_mod->name, mod_ret ? "OK" : "Fail");
      }
    }
  }

  return ret;
}


#if CLI_USE(HW_MODULE)
void cliModule(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("count : %d\n", (int)info.count);
    cliPrintf("%-4s %-16s %s\n", "idx", "name", "pri");

    for (int i = 0; i < info.count; i++)
    {
      cliPrintf("%-4d %-16s %d\n", i, info.p_module[i].name, (int)info.p_module[i].priority);
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("module info\n");
  }
}
#endif
