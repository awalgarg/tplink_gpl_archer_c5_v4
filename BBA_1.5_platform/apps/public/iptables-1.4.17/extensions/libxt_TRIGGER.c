/* Port-triggering target. 
 *
 * Copyright (C) 2003, CyberTAN Corporation
 * All Rights Reserved.
 */

/* Shared library add-on to iptables to add port-trigger support. */

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <iptables.h>
//#include <linux/netfilter/x_tables.h>
//#include <net/netfilter/nf_nat_rule.h>
#include <linux/netfilter/nf_nat.h>
#include <linux/netfilter_ipv4/ipt_TRIGGER.h>

/* Function which prints out usage message. */
static void 
TRIGGER_help(void)
{
	printf(
"TRIGGER options:\n"
" --trigger-type (dnat|in|out)\n"
"				Trigger type\n"
" --trigger-proto proto\n"
"				Trigger protocol\n"
" --trigger-match port[-port]\n"
"				Trigger destination port range\n"
" --trigger-relate port[-port]\n"
"				Port range to map related destination port range to.\n");
}

static const struct option TRIGGER_opts[] = {
	{ .name = "trigger-type", .has_arg = true, .val = '1'},
    { .name = "trigger-proto", .has_arg = true, .val = '2'},
	{ .name = "trigger-match", .has_arg = true, .val = '3'},	
	{ .name = "trigger-relate", .has_arg = true, .val = '4'},		
    XT_GETOPT_TABLEEND,
};

#if 0
/* Initialize the target. */
static void
TRIGGER_init(struct xt_entry_target *t, unsigned int *nfcache)
{
	/* Can't cache this */
	*nfcache |= NFC_UNKNOWN;
}
#endif

/* Parses ports */
static void
parse_ports(const char *arg, u_int16_t *ports)
{
	const char *dash;
	int port;

	port = atoi(arg);
	if (port == 0 || port > 65535)
		xtables_error(PARAMETER_PROBLEM, "Port range `%s' invalid\n", arg);

	dash = strchr(arg, '-');
	if (!dash)
		ports[0] = ports[1] = port;
	else {
		int maxport;

		maxport = atoi(dash + 1);
		if (maxport == 0 || maxport > 65535)
			xtables_error(PARAMETER_PROBLEM,
				   "Port range `%s' invalid\n", dash+1);
		if (maxport < port)
			xtables_error(PARAMETER_PROBLEM,
				   "Port range `%s' invalid\n", arg);
		ports[0] = port;
		ports[1] = maxport;
	}
}


/* Function which parses command options; returns true if it
   ate an option */
static int
TRIGGER_parse(int c, char **argv, int invert, unsigned int *flags,
      const void *entry,
      struct xt_entry_target **target)
{
	struct xt_trigger_info *info = (struct xt_trigger_info *)(*target)->data;

	switch (c) {
	case '1':
		if (!strcasecmp(optarg, "dnat"))
			info->type = XT_TRIGGER_DNAT;
		else if (!strcasecmp(optarg, "in"))
			info->type = XT_TRIGGER_IN;
		else if (!strcasecmp(optarg, "out"))
			info->type = XT_TRIGGER_OUT;
		else
			xtables_error(PARAMETER_PROBLEM,
				   "unknown type `%s' specified", optarg);
		return 1;

	case '2':
		if (!strcasecmp(optarg, "tcp"))
			info->proto = IPPROTO_TCP;
		else if (!strcasecmp(optarg, "udp"))
			info->proto = IPPROTO_UDP;
		else if (!strcasecmp(optarg, "all"))
			info->proto = 0;
		else
			xtables_error(PARAMETER_PROBLEM,
				   "unknown protocol `%s' specified", optarg);
		return 1;

	case '3':
		parse_ports(optarg, info->ports.mport);
		return 1;

	case '4':
		parse_ports(optarg, info->ports.rport);
		return 1;

	default:
		return 0;
	}
}

/* Final check; don't care. */
static void TRIGGER_final_check(unsigned int flags)
{
}

/* Prints out the targinfo. */
static void
TRIGGER_print(const void *ip,
      const struct xt_entry_target *target,
      int numeric)
{
	struct ipt_trigger_info *info = (struct xt_trigger_info *)target->data;

	printf("TRIGGER ");
	if (info->type == XT_TRIGGER_DNAT)
		printf("type:dnat ");
	else if (info->type == XT_TRIGGER_IN)
		printf("type:in ");
	else if (info->type == XT_TRIGGER_OUT)
		printf("type:out ");

	if (info->proto == IPPROTO_TCP)
		printf("tcp ");
	else if (info->proto == IPPROTO_UDP)
		printf("udp ");

	printf("match:%hu", info->ports.mport[0]);
	if (info->ports.mport[1] > info->ports.mport[0])
		printf("-%hu", info->ports.mport[1]);
	printf(" ");

	printf("relate:%hu", info->ports.rport[0]);
	if (info->ports.rport[1] > info->ports.rport[0])
		printf("-%hu", info->ports.rport[1]);
	printf(" ");
}

/* Saves the union ipt_targinfo in parsable form to stdout. */
static void
TRIGGER_save(const void*ip, const struct xt_entry_target *target)
{
	struct xt_trigger_info *info = (struct xt_trigger_info *)target->data;
		
	printf("--trigger-proto ");
	if (info->proto == IPPROTO_TCP)
		printf("tcp ");
	else if (info->proto == IPPROTO_UDP)
		printf("udp ");
	printf("--trigger-match %hu-%hu ", info->ports.mport[0], info->ports.mport[1]);
	printf("--trigger-relate %hu-%hu ", info->ports.rport[0], info->ports.rport[1]);
}

struct xtables_target trigger = { 
	//.next			= NULL,
	.family			= NFPROTO_IPV4,
    .name			= "TRIGGER",
    .version		= XTABLES_VERSION,
    .size			= XT_ALIGN(sizeof(struct ipt_trigger_info)),
    .userspacesize	= XT_ALIGN(sizeof(struct ipt_trigger_info)),
    .help			= TRIGGER_help,
    .init			= NULL,
    .parse			= TRIGGER_parse,
    .final_check	= TRIGGER_final_check,
    .print			= TRIGGER_print,
    .save			= TRIGGER_save,
    .extra_opts		= TRIGGER_opts
};

void _init(void)
{
	xtables_register_target(&trigger);
}
