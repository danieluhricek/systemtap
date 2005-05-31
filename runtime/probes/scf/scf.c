#define STP_NETLINK_ONLY
#define STP_NUM_STRINGS 2
#include "runtime.h"

#define MAP_STRING_LENGTH 512
#define KEY1_TYPE STRING
#include "map-keys.c"

#define VALUE_TYPE INT64
#include "map-values.c"

#include "map.c"
#include "sym.c"
#include "current.c"
#include "stack.c"
#include "probes.c"

MODULE_DESCRIPTION("SystemTap probe: scf");
MODULE_AUTHOR("Martin Hunt <hunt@redhat.com>");

MAP map1;

int inst_smp_call_function (struct kprobe *p, struct pt_regs *regs)
{
  String str = _stp_string_init (0);
  _stp_stack_sprint (str,regs);
  _stp_map_key_str(map1, _stp_string_ptr(str));
  _stp_map_add_int64 (map1, 1);
  return 0;
}

static struct kprobe stp_probes[] = {
  {
    .addr = (kprobe_opcode_t *)"smp_call_function",
    .pre_handler = inst_smp_call_function
  },
};

#define MAX_STP_ROUTINE (sizeof(stp_probes)/sizeof(struct kprobe))

static int pid;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "daemon pid");

int init_module(void)
{
  int ret;
  
  if (!pid) {
	  printk("init: Can't start without daemon pid\n");		
	  return -1;
  }

  if (_stp_transport_open(n_subbufs, subbuf_size, pid) < 0) {
	  printk("init: Couldn't open transport\n");		
	  return -1;
  }

  map1 = _stp_map_new_str (100, INT64);

  ret = _stp_register_kprobes (stp_probes, MAX_STP_ROUTINE);

  return ret;
}

static void probe_exit (void)
{
  _stp_unregister_kprobes (stp_probes, MAX_STP_ROUTINE);
  _stp_map_print (map1, "trace[%1s] = %d\n");
  _stp_map_del (map1);
}

void cleanup_module(void)
{
  _stp_transport_close();
}

MODULE_LICENSE("GPL");

