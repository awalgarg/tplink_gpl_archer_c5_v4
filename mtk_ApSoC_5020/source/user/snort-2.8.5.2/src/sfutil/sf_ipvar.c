/*
** Copyright (C) 1998-2009 Sourcefire, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
 * Adam Keeton
 * sf_ipvar.c
 * 11/17/06
 *
 * Library for IP variables.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "util.h"
#include "sf_ipvar.h"
#include "sf_vartable.h"

#define LIST_OPEN '['
#define LIST_CLOSE ']'

static SFIP_RET sfvar_list_compare(sfip_node_t *, sfip_node_t *);

static INLINE sfip_var_t *_alloc_var(void)
{
    return (sfip_var_t*)calloc(1, sizeof(sfip_var_t));
}

void sfvar_free(sfip_var_t *var)
{
    sfip_node_t *p, *next;

    if(!var) return;
    
    if(var->name) free(var->name);

    if(var->mode == SFIP_LIST)
    {
        for(p=var->head; p; p=next)
        {
            next = p->next;
            if(p->ip) sfip_free(p->ip);
            free(p);
        }

        for(p=var->neg_head; p; p=next)
        {
            next = p->next;
            if(p->ip) sfip_free(p->ip);
            free(p);
        }
    }
    else if(var->mode == SFIP_TABLE)
    {
        // XXX
    }

    free(var);
}

sfip_node_t *sfipnode_alloc(char *str, SFIP_RET *status)
{
    sfip_node_t *ret;

    if(!str)
    {
        if(status)
            *status = SFIP_ARG_ERR;
        return NULL;
    }

    if( (ret = (sfip_node_t*)calloc(1, sizeof(sfip_node_t))) == NULL )
    {
        if(status)
            *status = SFIP_ALLOC_ERR;
         return NULL;
    }
    
    /* Check if this string starts with a '!', if so,
     * then the node needs to be negated */
    if(*str == '!')
    {
        str++;
        ret->flags |= SFIP_NEGATED;
    } 
    
    /* Check if this is an "any" */
    if(!strncasecmp(str, "any", 3))
    {
        /* Make sure they're not doing !any, which is meaningless */
        if(ret->flags & SFIP_NEGATED)
        {
            if(status)
                *status = SFIP_ARG_ERR;
            free(ret);
            return NULL;
        }

        ret->flags |= SFIP_ANY;

        if( (ret->ip = sfip_alloc("0.0.0.0", status)) == NULL )
        {
            /* Failed to parse this string, so free and return */
            if(status)
                *status = SFIP_ALLOC_ERR; 

            free(ret);
            return NULL;
        }

        if(status)
            *status = SFIP_SUCCESS;

#if 0
        if( (ret->ip = sfip_alloc("0.0.0.0", NULL)) == NULL)
        {
            if(status)
                *status = SFIP_FAILURE;
            free(ret);
            return NULL;
        }
#endif

    } 
    else if( (ret->ip = sfip_alloc(str, status)) == NULL )
    {
        /* Failed to parse this string, so free and return */
        if(status)
             *status = SFIP_INET_PARSE_ERR;
        free(ret);
        return NULL;
    }

    /* Check if this is a negated, zero'ed IP (equivalent of a "!any") */
    if(!sfip_is_set(ret->ip) && (ret->flags & SFIP_NEGATED))
    {
        if(status)
            *status = SFIP_NOT_ANY;
        free(ret->ip);
        free(ret);
        return NULL;        
    }

    return ret;
}

/* Deep copy of src added to dst */
/* Ordering is not necessarily preserved */
SFIP_RET sfvar_add(sfip_var_t *dst, sfip_var_t *src)
{
    sfip_node_t *oldhead, *oldneg, *idx;
    sfip_var_t *copiedvar;

    if(!dst || !src) return SFIP_ARG_ERR;

    oldhead = dst->head;
    oldneg = dst->neg_head;

    if((copiedvar = sfvar_deep_copy(src)) == NULL)
    {
        return SFIP_ALLOC_ERR;
    }
    
    dst->head = copiedvar->head;
    dst->neg_head = copiedvar->neg_head;

    free(copiedvar);

    if(dst->head)
    {
        for(idx = dst->head; idx->next; idx = idx->next) 
            ;

        idx->next = oldhead;
    } 
    else
    {
        dst->head = oldhead;
    }

    if(dst->neg_head)
    {
        for(idx = dst->neg_head; idx->next; idx = idx->next) 
            ;

        idx->next = oldneg; 
    }
    else
    {
        dst->neg_head = oldneg;
    }

    return SFIP_SUCCESS;
}

SFIP_RET sfvar_add_node(sfip_var_t *var, sfip_node_t *node, int negated)
{
    sfip_node_t *p;
    sfip_node_t *swp;
    sfip_node_t **head;

    if(!var || !node) return SFIP_ARG_ERR;
    
    /* XXX */
    /* As of this writing, 11/20/06, nodes are always added to 
     * the list, regardless of the mode (list or table). */

    if(negated)
        head = &var->neg_head;        
    else
        head = &var->head;
        

    if(!(*head))
    {
        *head = node;
        return SFIP_SUCCESS;
    }

    /* "Anys" should always be inserted first */
    /* Otherwise, check if this IP is less than the head's IP */
    if((node->flags & SFIP_ANY) || 
       (sfip_compare(node->ip, (*head)->ip) == SFIP_LESSER))
    {
        node->next = *head;
        *head = node;
        return SFIP_SUCCESS;
    }

    /* If we're here, the head node was lesser than the new node */
    /* Before searching the list, verify there is atleast two nodes.
     * (This saves an extra check during the loop below) */
    if(!(*head)->next)
    {
        (*head)->next = node;
        return SFIP_SUCCESS;
    }

    /* Insertion sort */
    for(p = *head; p->next; p=p->next)
    {
        if(sfip_compare(node->ip, p->next->ip) == SFIP_LESSER)
        {
            swp = p->next;
            p->next = node;
            node->next = swp;

            return SFIP_SUCCESS;
        }
    }

    p->next = node;

    return SFIP_SUCCESS;

    /* XXX Insert new node into routing table */
//    sfrt_add(node->ip, 
}

static SFIP_RET sfvar_list_compare(sfip_node_t *list1, sfip_node_t *list2)
{
    int total1 = 0;
    int total2 = 0;
    char *usage;
    sfip_node_t *tmp;

    if ((list1 == NULL) && (list2 == NULL))
        return SFIP_EQUAL;

    /* Check the ip lists for count mismatch */
    for (tmp = list1; tmp != NULL; tmp = tmp->next) 
        total1++;
    for (tmp = list2; tmp != NULL; tmp = tmp->next) 
        total2++;
    if (total1 != total2) 
        return SFIP_FAILURE;

    /* Walk first list.  For each node, check if there is an equal
     * counterpart in the second list.  This method breaks down of there are 
     * duplicated nodes.  For instance, if one = {a, b} and two = {a, a}.
     * Therefore, need additional data structure[s] ('usage') to check off 
     * which nodes have been accounted for already. 
     *
     * Also, the lists are not necessarily ordered, so comparing 
     * node-for-node won't work */

    /* Lists are of equal size */
    usage = (char*)SnortAlloc(total1);

    for (tmp = list1; tmp != NULL; tmp = tmp->next)
    {
        int i, match = 0;
        sfip_node_t *tmp2;

        for (tmp2 = list2, i = 0; tmp2 != NULL; tmp2 = tmp2->next, i++)
        {
            if ((sfip_compare(tmp->ip, tmp2->ip) == SFIP_EQUAL) && !usage[i])
            {
                match = 1;
                usage[i] = 1;
                break;
            }
        }

        if(!match)
        {
            free(usage);
            return SFIP_FAILURE;
        }
    }

    free(usage);
    return SFIP_EQUAL;
}

/* Check's if two variables have the same nodes */
SFIP_RET sfvar_compare(sfip_var_t *one, sfip_var_t *two)
{
    /* If both NULL, consider equal */
    if(!one && !two)
        return SFIP_EQUAL;

    /* If one NULL and not the other, consider unequal */
    if((one && !two) || (!one && two)) 
        return SFIP_FAILURE;

    if (sfvar_list_compare(one->head, two->head) == SFIP_FAILURE)
        return SFIP_FAILURE;

    if (sfvar_list_compare(one->neg_head, two->neg_head) == SFIP_FAILURE)
        return SFIP_FAILURE;

    return SFIP_EQUAL;
}

/* Support function for sfvar_parse_iplist.  Used to 
 * correctly match up end brackets.  
 *  (Can't just do strchr(str, ']') because of the 
 *  [a, [b], c] case, and can't do strrchr because
 *  of the [a, [b], [c]] case) */
char *_find_end_token(char *str)
{
    int stack = 0;

    for(; *str; str++)
    {
        if(*str == LIST_OPEN)
            stack++;
        else if(*str == LIST_CLOSE)
            stack--;

        if(!stack)
        {
            return str;
        }
    }

    return NULL;
}

/* Support function for sfvar_parse_iplist.
 *  Negates a node */
static void _negate_node(sfip_node_t *node)
{
    if(node->addr_flags & SFIP_NEGATED)
    {
        node->addr_flags &= ~SFIP_NEGATED;
        node->flags &= ~SFIP_NEGATED;
    }
    else
    {
        node->addr_flags |= SFIP_NEGATED;
        node->flags |= SFIP_NEGATED;
    }
}

/* Support function for sfvar_parse_iplist.
 *  Negates a variable */
static void _negate_lists(sfip_var_t *var)
{
    sfip_node_t *node;
    sfip_node_t *temp;

    for(node = var->head; node; node=node->next) 
        _negate_node(node);

    for(node = var->neg_head; node; node=node->next) 
        _negate_node(node);
    
    /* Swap lists */
    temp = var->head;
    var->head = var->neg_head;
    var->neg_head = temp;
}

SFIP_RET sfvar_parse_iplist(vartable_t *table, sfip_var_t *var,
                           char *str, int negation)
{
    char *end, *tok;
    SFIP_RET ret;
    int neg_ip;

    if(!var || !table || !str) 
        return SFIP_ARG_ERR;

    while(*str)
    {
        /* Skip whitespace and leading commas */
        if(isspace((int)*str) || *str == ',')
        {
            str++;
            continue;
        }

        neg_ip = 0;

        /* Handle multiple negations */
        for(; *str == '!'; str++) 
             neg_ip = !neg_ip;

        /* Find end of this token */
        for(end = str+1; 
           *end && !isspace((int)*end) && *end != LIST_CLOSE && *end != ',';
            end++) ;

        tok = SnortStrndup(str, end - str);

        if(*str == LIST_OPEN)
        {
            char *list_tok;

            /* Find end of this list */
            if((end = _find_end_token(str)) == NULL)
            {
                /* No trailing bracket found */
                return SFIP_UNMATCHED_BRACKET;
            }
        
            str++;
            list_tok = SnortStrndup(str, end - str);

            if((ret = sfvar_parse_iplist(table, var, list_tok,
                           negation ^ neg_ip)) != SFIP_SUCCESS)
            {
                free(list_tok);
                free(tok);
                return ret;
            }

            free(list_tok);
        } 
        else if(*str == '$')
        {
            sfip_var_t *tmp_var;
            sfip_var_t *copy_var;

            if((tmp_var = sfvt_lookup_var(table, tok)) == NULL)
            {
                free(tok);
                return SFIP_LOOKUP_FAILURE;
            }

            copy_var = sfvar_deep_copy(tmp_var);
            /* Apply the negation */
            if(negation ^ neg_ip)
            {
                /* Check for a negated "any" */
                if(copy_var->head && copy_var->head->flags & SFIP_ANY)
                {
                    free(tok);
                    sfvar_free(copy_var);
                    return SFIP_NOT_ANY;
                }

                /* Check if this is a negated, zero'ed IP (equivalent of a "!any") */
                if(copy_var->head && !sfip_is_set(copy_var->head->ip))
                {
                    free(tok);
                    sfvar_free(copy_var);
                    return SFIP_NOT_ANY;
                }

                _negate_lists(copy_var);
            }

            sfvar_add(var, copy_var);
            sfvar_free(copy_var);
        } 
        else if(*str == LIST_CLOSE)
        {
            /* This should be the last character, if not, then this is an
             * invalid extra closing bracket */
            if(!(*(str+1)))
            {
                free(tok);
                return SFIP_SUCCESS; 
            }

            free(tok);
            return SFIP_UNMATCHED_BRACKET;
        }
        else
        {
            sfip_node_t *node;

            /* Skip leading commas */
            for(; *str == ','; str++) ;

            /* Check for a negated "any" */
            if(negation ^ neg_ip && !strcasecmp(tok, "any"))
            {
                free(tok);
                return SFIP_NOT_ANY;
            }
    
            /* This should be an IP address! */
            /* Allocate new node for this string and add it to "ret" */
            if((node = sfipnode_alloc(tok, &ret)) == NULL)
            {     
                free(tok);
                return ret;
            }

            if(negation ^ neg_ip)
            {
                _negate_node(node);
            }

            /* Check if this is a negated, zero'ed IP (equivalent of a "!any") */
            if(!sfip_is_set(node->ip) && (node->flags & SFIP_NEGATED))
            {
                free(tok);
                return SFIP_NOT_ANY;
            }

            ret = sfvar_add_node(var, node, negation ^ neg_ip);

            if(ret != SFIP_SUCCESS )
            {
                free(tok);
                return ret;
            }
        }

        free(tok);
        if(*end)
            str = end + 1;
        else break;
    }

    return SFIP_SUCCESS;
}

SFIP_RET sfvar_validate(sfip_var_t *var)
{
    sfip_node_t *idx, *neg_idx;
    
    if(!var->head || !var->neg_head)
        return SFIP_SUCCESS;
    
    for(idx = var->head; idx; idx = idx->next) 
    {
        for(neg_idx = var->neg_head; neg_idx; neg_idx = neg_idx->next)
        {
            /* A smaller netmask means "less specific" */
            if(sfip_bits(neg_idx->ip) <= sfip_bits(idx->ip) &&
                /* Verify they overlap */
                (sfip_contains(neg_idx->ip, idx->ip) == SFIP_CONTAINS))
            {
                return SFIP_CONFLICT;
            }
        }
    }
    
    return SFIP_SUCCESS;
}

/* Allocates and returns a new variable, described by "variable". */
sfip_var_t *sfvar_alloc(vartable_t *table, char *variable, SFIP_RET *status)
{
    sfip_var_t *ret;
    char *str, *end;
    
    if(!variable || !(*variable))
    {
        if(status)
            *status = SFIP_ARG_ERR;
        return NULL;
    }
    
    if( (ret = _alloc_var()) == NULL )
    {
        if(status)
            *status = SFIP_ALLOC_ERR;
        return NULL;
    }

    /* Extract and save the variable's name */
    /* Start by skipping leading whitespace or line continuations: '\' */
    for(str = variable ; *str && (isspace((int)*str) || *str == '\\'); str++) ;

    /* Find the end of the name */
    for(end = str; *end && !isspace((int)*end) && *end != '\\'; end++) ;

    if(!isalnum((int)*str) && *str != '$' && *str != '!')
    {
        if(status)
            *status = SFIP_ARG_ERR;

         return NULL;
    }

    /* Set the new variable's name/key */
    if((ret->name = SnortStrndup(str, end - str)) == NULL)
    {
        if(status)
            *status = SFIP_ALLOC_ERR;

        return NULL;
    }

    /* End points to the end of the name.  Skip past it */
    str = end;

    /* Everything is treated as a list, even if it's one element that's not
     * surrounded by brackets */
    if((*status = sfvar_parse_iplist(table, ret, str, 0)) != SFIP_SUCCESS)
    {
        return NULL;
    }

    if(ret->head && 
       (ret->head->flags & SFIP_ANY && ret->head->flags & SFIP_NEGATED))
       {
        if(status)
            *status = SFIP_NOT_ANY;

        sfvar_free(ret);
        return NULL;
    }

    if(sfvar_validate(ret) == SFIP_CONFLICT)
    {
        if(status)
            *status = SFIP_CONFLICT;

        sfvar_free(ret);
        return NULL;
    }

    return ret;
}

static INLINE sfip_node_t *_sfvar_deep_copy_list(sfip_node_t *idx)
{
    sfip_node_t *ret, *temp, *prev;

    ret = temp = NULL;
    
    for( ; idx; idx = idx->next)
    {

        prev = temp;

        if( (temp = (sfip_node_t*)calloc(1, sizeof(sfip_node_t))) == NULL )
        {
 // XXX WALK LIST AND DELETE NODES FIRST
            return NULL;
        }
        if( (temp->ip = (sfip_t*)calloc(1, sizeof(sfip_t))) == NULL )
        {
 // XXX WALK LIST AND DELETE NODES FIRST
            return NULL;
        }

        temp->flags = idx->flags;
        temp->addr_flags = idx->addr_flags;

        /* If it's an "any", there may be no IP object */
        if(idx->ip)
            memcpy(temp->ip, idx->ip, sizeof(sfip_t));

        if(prev)
            prev->next = temp;
        else
        {
            ret = temp;
        }
    } 
    return ret;
}

sfip_var_t *sfvar_deep_copy(sfip_var_t *var)
{
    sfip_var_t *ret;

    if(!var)
        return NULL;

    ret = (sfip_var_t*)SnortAlloc(sizeof(sfip_var_t));

    ret->mode = var->mode;
    ret->head = _sfvar_deep_copy_list(var->head);
    ret->neg_head = _sfvar_deep_copy_list(var->neg_head);

    return ret;
}


/* Support function for sfvar_ip_in  */
static INLINE int _sfvar_ip_in4(sfip_var_t *var, sfip_t *ip)
{
    int match;
    sfip_node_t *pos_idx, *neg_idx;

    match = 0;

    pos_idx = var->head;
    neg_idx = var->neg_head;

    if(!pos_idx) 
    {
        for( ; neg_idx; neg_idx = neg_idx->next) 
        {
            if(sfip_family(neg_idx->ip) != AF_INET) 
                continue;
        
            if(sfip_fast_cont4(neg_idx->ip, ip))
            {
                return 0;
            }
        } 

        return 1;
    }
    
    while(pos_idx)              
    {
        if(neg_idx)
        {
            if(sfip_family(neg_idx->ip) == AF_INET &&
                sfip_fast_cont4(neg_idx->ip, ip))
            {
                return 0;
            }
        
            neg_idx = neg_idx->next;
        } 
        /* No more potential negations.  Check if we've already matched. */
        else if(match)
        {
            return 1;
        }

        if(!match) 
        {
            if(sfip_is_set(pos_idx->ip))
            {
                if(sfip_family(pos_idx->ip) == AF_INET && 
                   sfip_fast_cont4(pos_idx->ip, ip))
                {
                    match = 1;
                }
                else
                {
                    pos_idx = pos_idx->next;
                }
            }
            else
            {
                match = 1;
            }
        }
    } 

    return 0;
}

/* Support function for sfvar_ip_in  */
static INLINE int _sfvar_ip_in6(sfip_var_t *var, sfip_t *ip)
{
    int match;
    sfip_node_t *pos_idx, *neg_idx;

    match = 0;

    pos_idx = var->head;
    neg_idx = var->neg_head;

    if(!pos_idx) 
    {
        for( ; neg_idx; neg_idx = neg_idx->next) 
        {
            if(sfip_family(neg_idx->ip) != AF_INET6) 
                continue;
        
            if(sfip_fast_cont6(neg_idx->ip, ip))
            {
                return 0;
            }
        } 

        return 1;
    }
    
    while(pos_idx)              
    {
        if(neg_idx)
        {
            if(sfip_family(neg_idx->ip) == AF_INET6 &&
                sfip_fast_cont6(neg_idx->ip, ip))
            {
                return 0;
            }
        
            neg_idx = neg_idx->next;
        } 
        /* No more potential negations.  Check if we've already matched. */
        else if(match)
        {
            return 1;
        }

        if(!match) 
        {
            if(sfip_is_set(pos_idx->ip))
            {

                if(sfip_family(pos_idx->ip) == AF_INET6 && 
                   sfip_fast_cont6(pos_idx->ip, ip))
                {
                     match = 1;
                }
                else
                {
                    pos_idx = pos_idx->next;
                }
            }
            else
            {
                match = 1;
            }
        }
    } 

    return 0;
}
/* Returns SFIP_SUCCESS if ip is contained in 'var', SFIP_FAILURE otherwise */
/* If either argument is NULL, SFIP_ARG_ERR is returned. */
int sfvar_ip_in(sfip_var_t *var, sfip_t *ip)
{
    if(!var || !ip)
        return 0;

#if 0
    if(var->mode == SFIP_TABLE)
    {
        // XXX
    }
    else
    {
#endif
        /* Since this is a performance-critical function it uses different
         * codepaths for IPv6 and IPv4 traffic, rather than the dual-stack 
         * functions. */

        if(sfip_family(ip) == AF_INET) 
        {
            return _sfvar_ip_in4(var, ip);
        }
        else 
        {
            return _sfvar_ip_in6(var, ip);
        }
#if 0
    }
#endif
}


/* Prints the variable "var" to the file descriptor 'f' */
void sfvar_print(FILE *f, sfip_var_t *var)
{
    if(!f) return;

    if(!var || !var->head)
    {
        fprintf(f, "[no variable]\n");
        return;
    }

    fprintf(f, "Name: %s\n", var->name);

    if(var->mode == SFIP_LIST)
    {
        if(var->head->flags & SFIP_ANY) 
            fprintf(f, "\t%p: <any>\n", (void*)var->head);
        else
        { 
            sfip_set_print(f, var->head);
        }
    }
    else if(var->mode == SFIP_TABLE)
    {
        // XXX
    }
}

void sfip_set_print(FILE *f, sfip_node_t *p)
{
     for(; p; p = p->next)
     {
         if(!p->ip) continue;
         if(p->flags & SFIP_NEGATED)
             fprintf(f, "\t!%s\n", sfip_to_str(p->ip));
         else
             fprintf(f, "\t %s\n", sfip_to_str(p->ip));
     }
}

int sfvar_flags(sfip_node_t *node)
{
    if(node) return node->flags;
    return -1;
}

/* XXX The unit tests for this code are performed within sf_vartable.c */
#if 0

int main()
{
    sfip_vtable *table;
    sfip_var_t *var;
    sfip_t *ip;
    
    /* Test parsing */
    /* Allowable arguments: 
     *      { <ip>[, <ip>, ... , <ip> }
     * Where an IP can be in CIDR notation, or be specified with a netmask.
     * IPs may also be negated with '!' */
    puts("********************************************************************");
    puts("Testing parsing:");
    var = sfvar_str(" {   1.2.3.4/8,  5.5.5.5 255.255.255.0, any} ");
    sfip_print_var(stdout, var);
    sfvar_free(var);
    puts("");
    var = sfvar_str(" {   1.2.3.4,  ffff::3/127, 0.0.0.1} ");
    sfip_print_var(stdout, var);
    ip = sfip_alloc("1.2.3.5");
    printf("(need more of these) 'in': %d\n", sfip_in(var, ip));
    puts("also, use 'sfip_in' for the unit tests");
    puts("");

    return 0;
}

#endif
    
