#include "tools.h"
#include "frat.h"

typedef struct mkpersist_{
    unhandle *uh;
    char alias[8];
}mkpersist_;

cmd_errout mkinit__(void **persistptr, char **argv, uint32_t argc){
    *persistptr = calloc(2, sizeof(void *));
    if(argc >= 2){
        ((void **)(*persistptr))[0] = fmount(argv + 1);
        ((void **)(*persistptr))[1] = calloc(MAXshellFHANDLES, sizeof(mkpersist_));
    }
    for(uint8_t cc = 0; cc < MAXshellFHANDLES; ++cc){memcpy(((mkpersist_ *)(*persistptr))[cc].alias, MKPERSIST_NOUSE, 8);}
    return *persistptr ? cmd_erroutSUCCESS: (cmd_errout){"Failed to Allocate Buffer", cmddescerrtype_outofmemory};
}

cmd_errout mkop__(size_t *buffer, void *persistptr){
    char *path = (char *)(buffer[0]),
        *name = (char *)(buffer[1]),
        sys = tolower((char)(buffer[2])),
        *alias = (char *)(buffer[3]);
    conf_fsroot *root = (mkpersist_ *)(((void **)persistptr)[0]);
    mkpersist_ *pbuffer = (mkpersist_ *)(((void **)persistptr)[1]);
    mkpersist_ *pitem = NULL;
    for(uint8_t cc = 0; cc < MAXshellFHANDLES; ++cc){
        if(!memcmp(pbuffer[cc].alias, MKPERSIST_NOUSE, 8)){
            pitem = pbuffer + cc;
        }
    }
    if(pitem){
        switch(sys){
            case 'd': {
                pitem->uh->dir = true;
                pitem->uh->dhandle_ = __floaddir(root, path, "dc");
                break;
            }
            case 'f': {
                pitem->uh->dir = false;
                pitem->uh->dhandle_ = fsloadh(root, path, "fc");
                break;
            }
            default: {break;}
        }
    }else{return (cmd_errout){.msg = "File Handle Buffer overflow Error, Unmount Files to Continue", .errcode = 1};}
    return cmd_erroutSUCCESS;
}

cmd_errout rmop__(size_t *buffer, void *persistptr){
    printf("\trmop");        return cmd_erroutSUCCESS;
}

cmd_errout wrop__(size_t *buffer, void *persistptr){
    printf("\twrop");        return cmd_erroutSUCCESS;
}

cmd_errout reop__(size_t *buffer, void *persistptr){
    printf("\treop");        return cmd_erroutSUCCESS;
}

cmd_errout mtinit__(void **persistptr){
    printf("\tmtinit");        return cmd_erroutSUCCESS;
}

cmd_errout mtop__(size_t *buffer, void *persistptr){
    printf("\tmtop");        return cmd_erroutSUCCESS;
}

cmd_errout utinit__(void **persistptr){
    printf("\tutinit");        return cmd_erroutSUCCESS;
}

cmd_errout utop__(size_t *buffer, void *persistptr){
    printf("\tutop");        return cmd_erroutSUCCESS;
}

cmd_errout ltinit__(void **persistptr){
    printf("\tltinit");        return cmd_erroutSUCCESS;
}

cmd_errout ltop__(size_t *buffer, void *persistptr){
    printf("\tltop");        return cmd_erroutSUCCESS;
}

cmd_errout exitop__(size_t *buffer, void *persistptr){printf("\texit");        exit(EXIT_SUCCESS);}

cmd_errout helpop__(size_t *buffer, void *persistptr){
    cmd_errout err = cmd_erroutSUCCESS;
    for(uint32_t cc = 0; cc < numcommands; ++cc){
        printf(
            "\nCommand: \"%.8s\"  Alias: \"%.8s\""
            "\nDesc:\n  %s"
            "\nFlags: ",
            commands[cc].cmd, commands[cc].alias, commands[cc].desc
        );
        for(uint8_t cc_ = 0; cc_ < commands[cc].numflags; ++cc_){
            if(commands[cc].flags[cc_].type){
                char *ftypename;
                switch(commands[cc].flags[cc_].type){
                    case cmddescargtype__switch: {ftypename = "switch";     break;}
                    case cmddescargtype__bool: {ftypename = "Boolean";     break;}
                    case cmddescargtype__char: {ftypename = "Char";     break;}
                    case cmddescargtype__byte: {ftypename = "Byte";     break;}
                    case cmddescargtype__word: {ftypename = "Word";     break;}
                    case cmddescargtype__dword: {ftypename = "Double-Word";     break;}
                    case cmddescargtype__qword: {ftypename = "Quad-Word";     break;}
                    case cmddescargtype__string: {ftypename = "String";     break;}
                    case cmddescargtype__longinteger: {ftypename = "Data";     break;}
                    default: {ftypename = "Undefined";      break;}
                }
                printf(
                    "\n\t[%u]: {"
                    "\n\t  Name: \"%.3s\""
                    "\n\t  Desc: %s"
                    "\n\t  Type: %u:%s",
                    cc_, commands[cc].flags[cc_].flag, commands[cc].flags[cc_].desc,
                    commands[cc].flags[cc_].type, ftypename
                );
            }
        }
    }
    return err;
}

static cmddesc *srchcmd(const char *cmd){
    char *temp = str_tolower(strdup(cmd));
    for(size_t i = 0; i < numcommands; i++){
        if(!strncmp(commands[i].cmd, temp, strlen(temp) - 1) || !strncmp(commands[i].alias, temp, strlen(temp) - 1)){
            free(temp);
            return &commands[i];
        }
    }
    free(temp);
    return NULL;
}

static cmddesc_flag *srchflag(cmddesc *desc, const char *flag){
    char *temp = str_tolower(strdup(flag));
    for(uint8_t i = 0; i < desc->numflags; i++){
        if(!strncmp(desc->flags[i].flag, temp, strlen(temp) - 1)){free(temp);      return &desc->flags[i];}
    }
    free(temp);
    return NULL;
}

static int argparse(cmddesc_argtype type, char *tok, size_t *out){
    char *temp = str_tolower(strdup(tok));
    switch(type){
        case cmddescargtype__switch: {
            *out = 1;
            return 1;
        }
        case cmddescargtype__bool: {
            if (!strncmp(temp, "true", 4)){*out = 1; free(temp); return 1;}
            if (!strncmp(temp, "false", 5)){*out = 0; free(temp); return 1;}
            return 0;
        }
        case cmddescargtype__char: {
            *out = (unsigned char)temp[0];
            free(temp);
            return 1;
        }
        case cmddescargtype__byte: {
            *out = (uint8_t)strtoul(temp, NULL, 0);
            free(temp);
            return 1;
        }
        case cmddescargtype__word: {
            *out = (uint16_t)strtoul(temp, NULL, 0);
            free(temp);
            return 1;
        }
        case cmddescargtype__dword: {
            *out = (uint32_t)strtoul(temp, NULL, 0);
            free(temp);
            return 1;
        }
        case cmddescargtype__qword: {
            *out = (uint64_t)strtoull(temp, NULL, 0);
            free(temp);
            return 1;
        }
        case cmddescargtype__string: {
            *out = (size_t)temp;
            free(temp);
            return 1;
        }
        case (cmddescargtype__longinteger | cmddescargtype__longdata): {
            *out = (size_t)temp;
            free(temp);
            return 1;
        }
        default: {free(temp);   return 0;}
    }
}

cmd_errout *shellinit(char **argc, uint32_t argv){
    cmd_errout *out = malloc(numcommands * sizeof(cmd_errout));
    for(uint8_t cc = 0; cc < numcommands; ++cc){
        out[cc] = commands[cc].init(&(commands[cc].persistent), argc, argv);
    }
    return out;
}

cmd_errout shell(char *in){
    cmd_errout err = cmd_erroutSUCCESS;

    if(!in || !*in){
        err.msg = "Empty input";
        err.errcode = 1;
        return err;
    }

    strtok_t *st;
    st = strtok_i(in, " \t", 0);

    /* First token = command */
    char *tok = strtok_k(st);
    if(!tok){
        strtok_d(st);
        err.msg = "No command found";
        err.errcode = 3;
        return err;
    }

    cmddesc *cmd = srchcmd(tok);
    if(!cmd){
        strtok_d(st);
        err.msg = "Unknown command";
        err.errcode = 4;
        return err;
    }

    size_t buffer[128];
    size_t bpos = 0;
    uint8_t next_positional = 0;

    while((tok = strtok_k(st))){

        /* Named parameter? */
        cmddesc_flag *named = srchflag(cmd, tok);
        if(named){
            char *arg = strtok_k(st);
            if(!arg){
                strtok_d(st);
                err.msg = "Parameter missing value";
                err.errcode = 5;
                return err;
            }

            if(named->type == cmddescargtype__longdata){
                buffer[bpos++] = (size_t)arg;
                break;
            }

            if(!argparse(named->type, arg, &buffer[bpos])){
                strtok_d(st);
                err.msg = "Invalid named parameter value";
                err.errcode = 6;
                return err;
            }

            bpos++;
            continue;
        }

        /* Positional parameter */
        if(next_positional >= cmd->numflags){
            strtok_d(st);
            err.msg = "Too many positional parameters";
            err.errcode = 7;
            return err;
        }

        cmddesc_flag *pos = &cmd->flags[next_positional];

        if(pos->type == cmddescargtype__longdata){
            buffer[bpos++] = (size_t)tok;
            break;
        }

        if(!argparse(pos->type, tok, &buffer[bpos])){
            strtok_d(st);
            err.msg = "Invalid positional parameter";
            err.errcode = 8;
            return err;
        }

        bpos++;
        next_positional++;
    }
    cmd->func(buffer, cmd->persistent);
    strtok_d(st);
    return err;
}