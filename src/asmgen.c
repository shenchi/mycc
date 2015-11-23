#include <stdio.h>
#include "memmgr.h"
#include "symmgr.h"
#include "qcode.h"
#include "ralloc.h"

FILE *fout = 0, *ftmp;

void outliteral(ident_t id){
    if(id->type!=LIT)return;
    if(id->subtype == TCHAR){
        fprintf(fout, "\'%c\'", (char)(id->data));
    }else if(id->subtype == TINT){
        fprintf(fout, "%d", id->data);
    }else if(id->subtype == TFLOAT){
        //fprintf(fout, "%f", (float)(id->data));
        fprintf(fout, "%s", (char*)(id->data));
    }else if(id->subtype == TSTR){
        fprintf(fout, "\'%s\'", (char*)(id->data));
    }
}

int localaddroff = 0;

void outref(ident_t id, int type){
    int addr = ((store_t)(id->extra))->addr;
    char *prefix;
    if(type==0)
        prefix = "";
    else
        prefix = "dword ptr ";
    if(id->loc == LOC_PARAM){
        fprintf(fout, "%s[EBP + %d]", prefix, 4 * (addr+2));

    }else if(id->loc == LOC_LOCAL){
        if(addr)
            fprintf(fout, "%s[ESP + %d]", prefix, (localaddroff + addr) * 4);
        else if(localaddroff)
			fprintf(fout, "%s[ESP + %d]", prefix, localaddroff*4);
		else
            fprintf(fout, "%s[ESP]", prefix);
    }else if(id->loc == LOC_GLOBAL){
        fprintf(fout, "%s[%s]", prefix, id->name);
    }
}

void outreg(int reg){
    char *name;
    if(reg == R_EAX)
        name = "EAX";
    else if(reg == R_EBX)
        name = "EBX";
    else if(reg == R_ECX)
        name = "ECX";
    else if(reg == R_EDX)
        name = "EDX";
    else if(reg == R_ESI)
        name = "ESI";
    else if(reg == R_EDI)
        name = "EDI";
    fprintf(fout, "%s", name);
}

// BIT_OP_TYPE:
// 0 - NULL
// 1 - REG
// 2 - ADDR
// 3 - LIT
// 4 - AUTO
// 5 - AUTO, NO REG
// 6 - AUTO, NO ADDR
// f = (BIT_OP1_TYPE << 4) + BIT_OP2_TYPE
void emit(char *inst, int f, ident_t op1, ident_t op2){
    int flag;
    fprintf(fout, "\t%s ", inst);
    if((flag = (f & 0xF0)>>4)){
        if(flag == 1)
            outreg(((store_t)(op1->extra))->cur_reg);
        else if(flag == 2)
            outref(op1, 1);
        else if(flag == 3)
            outliteral(op1);
		else if(flag >= 4){
			if(((store_t)(op1->extra))->cur_reg && flag!=5)
				outreg(((store_t)(op1->extra))->cur_reg);
			else if(((store_t)(op1->extra))->addr >= 0 && flag!=6)
				outref(op1, 1);
			else if(op1->type == LIT)
				outliteral(op1);
			// else error!
		}

        if((flag = f & 0x0F)){
            fprintf(fout, ", ");
            if(flag == 1)
                outreg(((store_t)(op1->extra))->cur_reg);
            else if(flag == 2)
                outref(op2, 1);
            else if(flag == 3)
                outliteral(op2);
			else if(flag >= 4){
				if(((store_t)(op2->extra))->cur_reg && flag!=5)
					outreg(((store_t)(op2->extra))->cur_reg);
				else if(((store_t)(op2->extra))->addr >= 0 && flag!=6)
					outref(op2, 1);
				else if(op2->type == LIT)
					outliteral(op2);
				// else error!
			}
        }
    }
    fprintf(fout, "\n");
}

void putlab(symtab_t tab, qcode_t code){
    symtab_t p = tab->next;
    while(p){
        if(p->entry->type == LABEL && p->entry->data == (int)code){
            fprintf(fout, "_%s:\n", p->entry->name);
        }
        p = p->next;
    }
}

int localsize;

void salloc(ident_t id){
	if(((store_t)(id->extra))->addr < 0 )
		((store_t)(id->extra))->addr = localsize++;
}

void sclear(){
    localsize = 0;
}

int current_index;

typedef struct paramstack_s * paramstack;
struct paramstack_s{
    ident_t param;
    paramstack next;
};
paramstack ps;
void ps_init(){ ps = 0; }
void ps_push(ident_t p){
    paramstack np = (paramstack)mmalloc(sizeof(struct paramstack_s));
    np->param = p;
    np->next = ps;
    ps = np;
}
ident_t ps_pop(){
    paramstack q;
    ident_t ret = 0;
    if(ps){
        ret = ps->param;
        q = ps;
        ps = ps->next;
        mfree(q);
    }
    return ret;
}

void processcode(func_t func){
    int r, f = 0, iotype;
    char *inst;
    ident_t p;
    qcode_t code = func->codes;
	current_index = 0;
	localaddroff = 0;
    ftmp = fout;
    fout = fopen("tmp", "w");
    ps_init();

    while(code){
		current_index = code->index;

        if(code->code == NOP){
            putlab(func->local, code);
        }else if(code->code == MOV){
            if(code->op1->subtype == TFLOAT && code->opr->subtype != TFLOAT){
                rfree(((store_t)(code->opr->extra))->cur_reg);
                salloc(code->opr);
                rfree(((store_t)(code->op1->extra))->cur_reg);
                emit("FLD", 0x20, code->op1, 0);
                emit("FIST", 0x20, code->opr, 0);

            }else if(code->opr->subtype == TFLOAT && code->op1->subtype != TFLOAT){
                rfree(((store_t)(code->opr->extra))->cur_reg);
                salloc(code->opr);
                rfree(((store_t)(code->op1->extra))->cur_reg);
                emit("FILD", 0x20, code->op1, 0);
                emit("FST", 0x20, code->opr, 0);

            }else{
                ralloc(code->opr, TEMP_REG);
                emit("MOV", 0x14, code->opr, code->op1);
				if(code->opr->subtype == TCHAR){
					fprintf(fout, "\tAND ");
					outreg(((store_t)(code->opr->extra))->cur_reg);
					fprintf(fout, ", 255\n");
				}
            }

        }else if(code->code == ADD || code->code == SUB){
			if(((store_t)(code->opr->extra))->cur_reg == 0){
				ralloc(code->opr, TEMP_REG);
				emit("MOVs", 0x14, code->opr, code->op1);
			}
			if(code->code == ADD)
				emit("ADD", 0x14, code->opr, code->op2);
			else
				emit("SUB", 0x14, code->opr, code->op2);
			if(code->opr->subtype == TFLOAT){
				rfree(((store_t)(code->opr->extra))->cur_reg);
				emit("FILD", 0x20, code->opr, 0);
				emit("FST", 0x20, code->opr, 0);
			}else if(code->opr->subtype == TCHAR){
				fprintf(fout, "\tAND ");
				outreg(((store_t)(code->opr->extra))->cur_reg);
				fprintf(fout, ", 255\n");
			}

        }else if(code->code == MUL){
			if(((store_t)(code->opr->extra))->cur_reg == 0){
				r = ralloc(code->opr, TEMP_REG);
				emit("MOV", 0x14, code->opr, code->op1);
			}else
				r = ((store_t)(code->opr->extra))->cur_reg;
			if(code->op2->type == LIT && ((store_t)(code->op2->extra))->cur_reg == 0){
				ralloc(code->op2, TEMP_REG & (~r));
				emit("MOV", 0x15, code->op2, code->op2);
			}
			emit("IMUL", 0x14, code->opr, code->op2);
			if(code->opr->subtype == TFLOAT){
				rfree(((store_t)(code->opr->extra))->cur_reg);
				emit("FILD", 0x20, code->opr, 0);
				emit("FST", 0x20, code->opr, 0);
			}else if(code->opr->subtype == TCHAR){
				fprintf(fout, "\tAND ");
				outreg(((store_t)(code->opr->extra))->cur_reg);
				fprintf(fout, ", 255\n");
			}

		}else if(code->code == DIV){
			rfree(R_EDX);
			fprintf(fout, "\tXOR EDX, EDX");
			ralloc(code->opr, R_EAX);
			emit("MOV", 0x14, code->opr, code->op1);
			if(code->op2->type == LIT && ((store_t)(code->op2->extra))->cur_reg == 0){
				ralloc(code->op2, TEMP_REG & (~R_EDX) & (~R_EAX));
				emit("MOV", 0x15, code->op2, code->op2);
			}
			emit("IDIV", 0x40, code->op2, 0);
			if(code->opr->subtype == TFLOAT){
				rfree(((store_t)(code->opr->extra))->cur_reg);
				emit("FILD", 0x20, code->opr, 0);
				emit("FST", 0x20, code->opr, 0);
			}else if(code->opr->subtype == TCHAR){
				fprintf(fout, "\tAND ");
				outreg(((store_t)(code->opr->extra))->cur_reg);
				fprintf(fout, ", 255\n");
			}

		}else if(code->code == CMP){
			if(((store_t)(code->op1->extra))->cur_reg == 0){
				ralloc(code->op1, TEMP_REG);
				emit("MOV", 0x15, code->op1, code->op1);
			}
			emit("CMP", 0x14, code->op1, code->op2);

		}else if(code->code == NEG){
			if(((store_t)(code->opr->extra))->cur_reg == 0){
				ralloc(code->opr, TEMP_REG);
				emit("MOV", 0x14, code->opr, code->op1);
			}
			emit("NEG", 0x10, code->opr, 0);
			if(code->opr->subtype == TFLOAT){
				rfree(((store_t)(code->opr->extra))->cur_reg);
				emit("FILD", 0x20, code->opr, 0);
				emit("FST", 0x20, code->opr, 0);
			}else if(code->opr->subtype == TCHAR){
				fprintf(fout, "\tAND ");
				outreg(((store_t)(code->opr->extra))->cur_reg);
				fprintf(fout, ", 255\n");
			}

		}else if(code->code == FADD || code->code == FSUB || code->code == FMUL || code->code == FDIV){
            if(!f){
                emit("FINIT", 0x00, 0, 0);
                f = 1;
            }

            if(((store_t)(code->op1->extra))->addr < 0){
                salloc(code->op1);
                emit("MOV", 0x26, code->op1, code->op1);
            }else if(((store_t)(code->op1->extra))->cur_reg){
                rfree(((store_t)(code->op1->extra))->cur_reg);
            }
            if(code->op1->subtype == TFLOAT)
                emit("FLD", 0x20, code->op1, 0);
            else
                emit("FILD", 0x20, code->op1, 0);

            if(((store_t)(code->op2->extra))->addr < 0){
                salloc(code->op2);
                emit("MOV", 0x26, code->op2, code->op2);
            }else if(((store_t)(code->op2->extra))->cur_reg){
                rfree(((store_t)(code->op2->extra))->cur_reg);
            }
            if(code->op2->subtype == TFLOAT){
                if(code->code == FADD)
                    inst = "FADD";
                else if(code->code == FSUB)
                    inst = "FSUB";
                else if(code->code == FMUL)
                    inst = "FMUL";
                else if(code->code == FDIV)
                    inst = "FDIV";
                emit(inst, 0x20, code->op2, 0);
            }else{
                if(code->code == FADD)
                    inst = "FIADD";
                else if(code->code == FSUB)
                    inst = "FISUB";
                else if(code->code == FMUL)
                    inst = "FIMUL";
                else if(code->code == FDIV)
                    inst = "FIDIV";
                emit(inst, 0x20, code->op2, 0);
            }

            salloc(code->opr);
            rfree(((store_t)(code->opr->extra))->cur_reg);
            emit("FST", 0x20, code->opr, 0);

		}else if(code->code == FCMP){
		    if(!f){
                emit("FINIT", 0x00, 0, 0);
                f = 1;
            }

		    if(((store_t)(code->op2->extra))->addr < 0){
                salloc(code->op2);
                emit("MOV", 0x26, code->op2, code->op2);
            }else if(((store_t)(code->op2->extra))->cur_reg){
                rfree(((store_t)(code->op2->extra))->cur_reg);
            }
            if(code->op2->subtype == TFLOAT)
                emit("FLD", 0x20, code->op2, 0);
            else
                emit("FILD", 0x20, code->op2, 0);

            emit("FST ST1", 0x00, 0, 0);

		    if(((store_t)(code->op1->extra))->addr < 0){
                salloc(code->op1);
                emit("MOV", 0x26, code->op1, code->op1);
            }else if(((store_t)(code->op1->extra))->cur_reg){
                rfree(((store_t)(code->op1->extra))->cur_reg);
            }
            if(code->op1->subtype == TFLOAT)
                emit("FLD", 0x20, code->op1, 0);
            else
                emit("FILD", 0x20, code->op1, 0);

            emit("FCOMI ST, ST1", 0x00, 0, 0);

		}else if(code->code == FNEG){
		    if(!f){
                emit("FINIT", 0x00, 0, 0);
                f = 1;
            }

		    if(((store_t)(code->op1->extra))->addr < 0){
                salloc(code->op1);
                emit("MOV", 0x26, code->op1, code->op1);
            }else if(((store_t)(code->op1->extra))->cur_reg){
                rfree(((store_t)(code->op1->extra))->cur_reg);
            }
            if(code->op1->subtype == TFLOAT)
                emit("FLD", 0x20, code->op1, 0);
            else
                emit("FILD", 0x20, code->op1, 0);

            emit("FCHS", 0x00, 0, 0);

            salloc(code->opr);
            rfree(((store_t)(code->opr->extra))->cur_reg);
            emit("FST", 0x20, code->opr, 0);

		}else if(code->code == JGT){
			rprotect();
			fprintf(fout, "\tJG\t_%s\n", code->op1->name);

		}else if(code->code == JGE){
			rprotect();
			fprintf(fout, "\tJGE\t_%s\n", code->op1->name);

		}else if(code->code == JLT){
			rprotect();
			fprintf(fout, "\tJL\t_%s\n", code->op1->name);

		}else if(code->code == JLE){
			rprotect();
			fprintf(fout, "\tJLE\t_%s\n", code->op1->name);

		}else if(code->code == JEQ){
			rprotect();
			fprintf(fout, "\tJE\t_%s\n", code->op1->name);

		}else if(code->code == JNE){
			rprotect();
			fprintf(fout, "\tJNE\t_%s\n", code->op1->name);

		}else if(code->code == JMP){
			rprotect();
			fprintf(fout, "\tJMP\t_%s\n", code->op1->name);

		}else if(code->code == PARAM){
		    ps_push(code->op1);
		    rfree(((store_t)(code->op1->extra))->cur_reg);

		}else if(code->code == CALL){
		    rprotect();
			localaddroff = 0;
            while((p = ps_pop())){
                fprintf(fout, "\tPUSH ");
                if(((store_t)(p->extra))->cur_reg)
                    outreg(((store_t)(p->extra))->cur_reg);
                else if(((store_t)(p->extra))->addr >= 0)
                    outref(p, 1);
                else if(p->type == LIT)
                    outliteral(p);
                fprintf(fout, "\n");
				localaddroff++;
            }
            ps_init();
			fprintf(fout, "\tCALL\t%s\n", code->op1->name);
			//if(((func_t)(code->op1->extra))->nparam > 0)
			//	fprintf(fout, "\tADD ESP, %d\n", 4 * ((func_t)(code->op1->extra))->nparam);
			localaddroff = 0;

			if(code->opr){
                ralloc(code->opr, TEMP_REG & (~R_EAX));
                fprintf(fout, "\tMOV ");
                outreg(((store_t)(code->opr->extra))->cur_reg);
                fprintf(fout, ", EAX\n");
			}

        }else if(code->code == RET){
			rreturn();

			if(code->op1){
				if(((store_t)(code->op1->extra))->cur_reg == 0){
					//ralloc(code->op1, R_EAX);
					((store_t)(code->op1->extra))->cur_reg = R_EAX;
					emit("MOV", 0x15, code->op1, code->op1);
					((store_t)(code->op1->extra))->cur_reg = 0;
				}else if(((store_t)(code->op1->extra))->cur_reg != R_EAX){
					fprintf(fout, "\tMOV EAX, ");
					outreg(((store_t)(code->op1->extra))->cur_reg);
					fprintf(fout, "\n");
				}
			}
            fprintf(fout, "\tLEAVE\n\tRET");
            if(func->nparam)
                fprintf(fout, " %d\n", 4 * func->nparam);
            else
                fprintf(fout, "\n");

        }else if(code->code == IN){
            if(code->opr->subtype == TCHAR)
                iotype = 1;
            else if(code->opr->subtype == TFLOAT)
                iotype = 2;
            else
                iotype = 0;
            rprotect();
			salloc(code->opr);
            fprintf(fout, "\tLEA EAX, ");
            outref(code->opr, 0);
            fprintf(fout, "\n\tPUSH EAX\n");
            fprintf(fout, "\tPUSH %d\n", iotype);
            fprintf(fout, "\tCALL\t%s\n", "_myccin");
            //fprintf(fout, "\tADD ESP, 8\n");

        }else if(code->code == OUT){
            if(code->op1->subtype == TCHAR)
                iotype = 1;
            else if(code->op1->subtype == TFLOAT)
                iotype = 2;
            else if(code->op1->subtype == TSTR)
                iotype = 3;
            else
                iotype = 0;

            rprotect();
            if(code->op1->subtype == TSTR){
                fprintf(fout, "\tLEA EAX, ");
                outref(code->op1, 0);
                fprintf(fout, "\n\tPUSH EAX\n");
                fprintf(fout, "\tPUSH %d\n", iotype);
                fprintf(fout, "\tCALL\t%s\n", "_myccout");
                //fprintf(fout, "\tADD ESP, 8\n");
            }else if(code->op1->subtype == TFLOAT){
                emit("FLD", 0x20, code->op1, 0);
                emit("PUSH 0", 0, 0, 0);
                emit("FST dword ptr [ESP]", 0, 0, 0);
				emit("PUSH 2", 0, 0, 0);
                fprintf(fout, "\tCALL\t%s\n", "_myccout");
                //fprintf(fout, "\tADD ESP, 4\n");
            }else{
                emit("PUSH", 0x40, code->op1, 0);
                fprintf(fout, "\tPUSH %d\n", iotype);
                fprintf(fout, "\tCALL\t%s\n", "_myccout");
                //fprintf(fout, "\tADD ESP, 8\n");
            }

        }
		fprintf(fout, "\n");
        code = code->next;
    }
    fclose(fout);
    fout = ftmp;
}

void outputcode(){
    char ch;
    ftmp = fopen("tmp", "r");
    while((ch=fgetc(ftmp))!=EOF)fputc(ch, fout);
    fclose(ftmp);
}

void processfunc(ident_t fentry){
    int i = 0;
    func_t func = fentry->extra;
	symtab_t t = func->local->next;
	param_t p = func->params;


    sclear();
    rclear();


    while(p){
        if(p->id->extra == 0){
            p->id->extra = mmalloc(sizeof(struct store_s));
            ((store_t)(p->id->extra))->wanted_reg = 0;
        }
        ((store_t)(p->id->extra))->cur_reg = 0;
        ((store_t)(p->id->extra))->addr = i++;
        p = p->next;
    }


    while(t){
        if(t->entry->type == VAR || t->entry->type == TEMP){
            if(!(t->entry->extra)){
                t->entry->extra = mmalloc(sizeof(struct store_s));
                ((store_t)(t->entry->extra))->wanted_reg = 0;
            }
            ((store_t)(t->entry->extra))->cur_reg = 0;
            ((store_t)(t->entry->extra))->addr = -1;
            if(t->entry->type == VAR)
               salloc(t->entry);
        }

        t = t->next;
    }

    fprintf(fout, "\n%s PROC\n", fentry->name);
    processcode(func);
    fprintf(fout, "\tENTER %d, 0\n\n", 4 * localsize);
    outputcode();
	fprintf(fout, "%s ENDP\n\n", fentry->name);
	rclear();
}

void preprocess(func_t func){
	qcode_t p = func->codes;
	int index = 0;
	while(p){
		p->index = index;
		if(p->op1 && p->op1->loc!=LOC_GLOBAL)p->op1->last = index;
		if(p->op2 && p->op2->loc!=LOC_GLOBAL)p->op2->last = index;
		index ++;
		p = p->next;
	}
}

int globalsize = 0;
void generate(char *filename){
	symtab_t t = global->next;
    store_t s;

	globalsize = 0;

    fout = fopen(filename, "w");

	fprintf(fout, "include \\masm32\\include\\masm32rt.inc\n\n\t.data\n");

    while(t){
        if(t->entry->type == CONSTANT || t->entry->type == VAR){
            if(t->entry->extra == 0){
                t->entry->extra = mmalloc(sizeof(struct store_s));
                s = (store_t)(t->entry->extra);
                s->wanted_reg = 0;
            }
            s->cur_reg = 0;
            s->addr = globalsize++;
			fprintf(fout, "%s\tdd\t", t->entry->name);
			if(t->entry->subtype == TFLOAT)
                if(t->entry->type == VAR)
                    fprintf(fout, "0.0\n");
                else
                    fprintf(fout, "%s\n", (char*)(t->entry->data));
			else
				fprintf(fout, "%d\n", t->entry->data);

        }
		if(t->entry->type == LIT){
            if(t->entry->extra == 0){
                t->entry->extra = mmalloc(sizeof(struct store_s));
                s = (store_t)(t->entry->extra);
                s->wanted_reg = 0;
            }
            s->cur_reg = 0;
            s->addr = -1;
            if(t->entry->subtype == TSTR){
                fprintf(fout, "%s\tdb\t", t->entry->name);
                outliteral(t->entry);
                fprintf(fout, ", 0\n");
                s->addr = globalsize++;
            }else
                t->entry->loc = LOC_LOCAL;
        }
		t = t->next;
    }

	fprintf(fout, "\n\t.code\n");
	fprintf(fout, "start:\n\tcall main\n\texit\n\ninclude mycclib.asm\n\n");
	t = global->next;
	while(t){
		if(t->entry->type == FUNC){
			preprocess((func_t)(t->entry->extra));
            processfunc(t->entry);
        }
		t = t->next;
	}

	fprintf(fout, "end start\n");
    fclose(fout);

}
