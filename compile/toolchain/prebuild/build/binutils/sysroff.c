#ifdef SYSROFF_PRINT
#include <stdio.h>
#include <stdlib.h>
#include <ansidecl.h>
void sysroff_print_cs_out (struct IT_cs *ptr)
{
itheader("cs", IT_cs_CODE);
tabout();
	printf("/*size                          */ ptr->size = ");
	printf("%d\n",ptr->size );
tabout();
	printf("/*hd                            */ ptr->hd = ");
	printf("%d\n",ptr->hd );
tabout();
	printf("/*hs                            */ ptr->hs = ");
	printf("%d\n",ptr->hs );
tabout();
	printf("/*un                            */ ptr->un = ");
	printf("%d\n",ptr->un );
tabout();
	printf("/*us                            */ ptr->us = ");
	printf("%d\n",ptr->us );
tabout();
	printf("/*sc                            */ ptr->sc = ");
	printf("%d\n",ptr->sc );
tabout();
	printf("/*ss                            */ ptr->ss = ");
	printf("%d\n",ptr->ss );
tabout();
	printf("/*er                            */ ptr->er = ");
	printf("%d\n",ptr->er );
tabout();
	printf("/*ed                            */ ptr->ed = ");
	printf("%d\n",ptr->ed );
tabout();
	printf("/*sh                            */ ptr->sh = ");
	printf("%d\n",ptr->sh );
tabout();
	printf("/*ob                            */ ptr->ob = ");
	printf("%d\n",ptr->ob );
tabout();
	printf("/*rl                            */ ptr->rl = ");
	printf("%d\n",ptr->rl );
tabout();
	printf("/*du                            */ ptr->du = ");
	printf("%d\n",ptr->du );
tabout();
	printf("/*dps                           */ ptr->dps = ");
	printf("%d\n",ptr->dps );
tabout();
	printf("/*dsy                           */ ptr->dsy = ");
	printf("%d\n",ptr->dsy );
tabout();
	printf("/*dty                           */ ptr->dty = ");
	printf("%d\n",ptr->dty );
tabout();
	printf("/*dln                           */ ptr->dln = ");
	printf("%d\n",ptr->dln );
tabout();
	printf("/*dso                           */ ptr->dso = ");
	printf("%d\n",ptr->dso );
tabout();
	printf("/*dus                           */ ptr->dus = ");
	printf("%d\n",ptr->dus );
tabout();
	printf("/*dss                           */ ptr->dss = ");
	printf("%d\n",ptr->dss );
tabout();
	printf("/*dbt                           */ ptr->dbt = ");
	printf("%d\n",ptr->dbt );
tabout();
	printf("/*dpp                           */ ptr->dpp = ");
	printf("%d\n",ptr->dpp );
tabout();
	printf("/*dfp                           */ ptr->dfp = ");
	printf("%d\n",ptr->dfp );
tabout();
	printf("/*den                           */ ptr->den = ");
	printf("%d\n",ptr->den );
tabout();
	printf("/*dds                           */ ptr->dds = ");
	printf("%d\n",ptr->dds );
tabout();
	printf("/*dar                           */ ptr->dar = ");
	printf("%d\n",ptr->dar );
tabout();
	printf("/*dpt                           */ ptr->dpt = ");
	printf("%d\n",ptr->dpt );
tabout();
	printf("/*dul                           */ ptr->dul = ");
	printf("%d\n",ptr->dul );
tabout();
	printf("/*dse                           */ ptr->dse = ");
	printf("%d\n",ptr->dse );
tabout();
	printf("/*dot                           */ ptr->dot = ");
	printf("%d\n",ptr->dot );
}
void sysroff_print_hd_out (struct IT_hd *ptr)
{
itheader("hd", IT_hd_CODE);
if (ptr->mt  == 0) { tabout(); printf("MTYPE_ABS_LM\n");}
if (ptr->mt  == 1) { tabout(); printf("MTYPE_REL_LM\n");}
if (ptr->mt  == 2) { tabout(); printf("MTYPE_OMS_OR_LMS\n");}
if (ptr->mt  == 0xf) { tabout(); printf("MTYPE_UNSPEC\n");}
tabout();
	printf("/*module type                   */ ptr->mt = ");
	printf("%d\n",ptr->mt );
tabout();
	printf("/*spare                         */ ptr->spare1 = ");
	printf("%d\n",ptr->spare1 );
tabout();
	printf("/*creation date                 */ ptr->cd = ");
	printf("%s\n",ptr->cd );
tabout();
	printf("/*number of units               */ ptr->nu = ");
	printf("%d\n",ptr->nu );
tabout();
	printf("/*code                          */ ptr->code = ");
	printf("%d\n",ptr->code );
tabout();
	printf("/*version                       */ ptr->ver = ");
	printf("%s\n",ptr->ver );
tabout();
	printf("/*address update                */ ptr->au = ");
	printf("%d\n",ptr->au );
tabout();
	printf("/*segment identifier            */ ptr->si = ");
	printf("%d\n",ptr->si );
tabout();
	printf("/*address field length          */ ptr->afl = ");
	printf("%d\n",ptr->afl );
tabout();
	printf("/*spare                         */ ptr->spare2 = ");
	printf("%d\n",ptr->spare2 );
tabout();
	printf("/*space size within segment     */ ptr->spcsz = ");
	printf("%d\n",ptr->spcsz );
tabout();
	printf("/*segment size                  */ ptr->segsz = ");
	printf("%d\n",ptr->segsz );
tabout();
	printf("/*segment shift                 */ ptr->segsh = ");
	printf("%d\n",ptr->segsh );
tabout();
	printf("/*entry point                   */ ptr->ep = ");
	printf("%d\n",ptr->ep );
	if (ptr->ep) {
	if (ptr->mt != MTYPE_ABS_LM) {
tabout();
	printf("/*unit appearance number        */ ptr->uan = ");
	printf("%d\n",ptr->uan );
tabout();
	printf("/*section appearance number     */ ptr->sa = ");
	printf("%d\n",ptr->sa );
	}
	if (segmented_p) {
tabout();
	printf("/*segment address               */ ptr->sad = ");
	printf("%d\n",ptr->sad );
	}
tabout();
	printf("/*address                       */ ptr->address = ");
	printf("%d\n",ptr->address );
	}
tabout();
	printf("/*os name                       */ ptr->os = ");
	printf("%s\n",ptr->os );
tabout();
	printf("/*sys name                      */ ptr->sys = ");
	printf("%s\n",ptr->sys );
tabout();
	printf("/*module name                   */ ptr->mn = ");
	printf("%s\n",ptr->mn );
tabout();
	printf("/*cpu                           */ ptr->cpu = ");
	printf("%s\n",ptr->cpu );
}
void sysroff_print_hs_out (struct IT_hs *ptr)
{
itheader("hs", IT_hs_CODE);
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
}
void sysroff_print_un_out (struct IT_un *ptr)
{
itheader("un", IT_un_CODE);
if (ptr->format  == 0) { tabout(); printf("FORMAT_LM\n");}
if (ptr->format  == 1) { tabout(); printf("FORMAT_OM\n");}
if (ptr->format  == 2) { tabout(); printf("FORMAT_OMS_OR_LMS\n");}
tabout();
	printf("/*format                        */ ptr->format = ");
	printf("%d\n",ptr->format );
tabout();
	printf("/*spare                         */ ptr->spare1 = ");
	printf("%d\n",ptr->spare1 );
tabout();
	printf("/*number of sections            */ ptr->nsections = ");
	printf("%d\n",ptr->nsections );
tabout();
	printf("/*number of external refs       */ ptr->nextrefs = ");
	printf("%d\n",ptr->nextrefs );
tabout();
	printf("/*number of external defs       */ ptr->nextdefs = ");
	printf("%d\n",ptr->nextdefs );
tabout();
	printf("/*unit name                     */ ptr->name = ");
	printf("%s\n",ptr->name );
tabout();
	printf("/*tool name                     */ ptr->tool = ");
	printf("%s\n",ptr->tool );
tabout();
	printf("/*creation date                 */ ptr->tcd = ");
	printf("%s\n",ptr->tcd );
tabout();
	printf("/*linker name                   */ ptr->linker = ");
	printf("%s\n",ptr->linker );
tabout();
	printf("/*creation date                 */ ptr->lcd = ");
	printf("%s\n",ptr->lcd );
}
void sysroff_print_us_out (struct IT_us *ptr)
{
itheader("us", IT_us_CODE);
tabout();
	printf("/*negotiation number            */ ptr->neg = ");
	printf("%d\n",ptr->neg );
}
void sysroff_print_sc_out (struct IT_sc *ptr)
{
itheader("sc", IT_sc_CODE);
tabout();
	printf("/*format                        */ ptr->format = ");
	printf("%d\n",ptr->format );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
tabout();
	printf("/*segment address               */ ptr->segadd = ");
	printf("%d\n",ptr->segadd );
tabout();
	printf("/*address                       */ ptr->addr = ");
	printf("%d\n",ptr->addr );
tabout();
	printf("/*length                        */ ptr->length = ");
	printf("%d\n",ptr->length );
tabout();
	printf("/*alignment                     */ ptr->align = ");
	printf("%d\n",ptr->align );
if (ptr->contents  == 0) { tabout(); printf("CONTENTS_CODE\n");}
if (ptr->contents  == 1) { tabout(); printf("CONTENTS_DATA\n");}
if (ptr->contents  == 2) { tabout(); printf("CONTENTS_STACK\n");}
if (ptr->contents  == 3) { tabout(); printf("CONTENTS_DUMMY\n");}
if (ptr->contents  == 4) { tabout(); printf("CONTENTS_SPECIAL\n");}
if (ptr->contents  == 0xf) { tabout(); printf("CONTENTS_NONSPEC\n");}
tabout();
	printf("/*contents                      */ ptr->contents = ");
	printf("%d\n",ptr->contents );
if (ptr->concat  == 0) { tabout(); printf("CONCAT_SIMPLE\n");}
if (ptr->concat  == 1) { tabout(); printf("CONCAT_SHAREDC\n");}
if (ptr->concat  == 2) { tabout(); printf("CONCAT_DUMMY\n");}
if (ptr->concat  == 3) { tabout(); printf("CONCAT_GROUP\n");}
if (ptr->concat  == 4) { tabout(); printf("CONCAT_SHARED\n");}
if (ptr->concat  == 5) { tabout(); printf("CONCAT_PRIVATE\n");}
if (ptr->concat  == 0xf) { tabout(); printf("CONCAT_UNSPEC\n");}
tabout();
	printf("/*concat                        */ ptr->concat = ");
	printf("%d\n",ptr->concat );
tabout();
	printf("/*read                          */ ptr->read = ");
	printf("%d\n",ptr->read );
tabout();
	printf("/*write                         */ ptr->write = ");
	printf("%d\n",ptr->write );
tabout();
	printf("/*exec                          */ ptr->exec = ");
	printf("%d\n",ptr->exec );
tabout();
	printf("/*initialized                   */ ptr->init = ");
	printf("%d\n",ptr->init );
tabout();
	printf("/*mode                          */ ptr->mode = ");
	printf("%d\n",ptr->mode );
tabout();
	printf("/*spare                         */ ptr->spare1 = ");
	printf("%d\n",ptr->spare1 );
tabout();
	printf("/*name                          */ ptr->name = ");
	printf("%s\n",ptr->name );
}
void sysroff_print_ss_out (struct IT_ss *ptr)
{
itheader("ss", IT_ss_CODE);
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
}
void sysroff_print_er_out (struct IT_er *ptr)
{
itheader("er", IT_er_CODE);
if (ptr->type  == 0) { tabout(); printf("ER_ENTRY\n");}
if (ptr->type  == 1) { tabout(); printf("ER_DATA\n");}
if (ptr->type  == 2) { tabout(); printf("ER_NOTDEF\n");}
if (ptr->type  == 3) { tabout(); printf("ER_NOTSPEC\n");}
tabout();
	printf("/*symbol type                   */ ptr->type = ");
	printf("%d\n",ptr->type );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
tabout();
	printf("/*symbol name                   */ ptr->name = ");
	printf("%s\n",ptr->name );
}
void sysroff_print_ed_out (struct IT_ed *ptr)
{
itheader("ed", IT_ed_CODE);
tabout();
	printf("/*section appearance number     */ ptr->section = ");
	printf("%d\n",ptr->section );
if (ptr->type  == 0) { tabout(); printf("ED_TYPE_ENTRY\n");}
if (ptr->type  == 1) { tabout(); printf("ED_TYPE_DATA\n");}
if (ptr->type  == 2) { tabout(); printf("ED_TYPE_CONST\n");}
if (ptr->type  == 7) { tabout(); printf("ED_TYPE_NOTSPEC\n");}
tabout();
	printf("/*symbol type                   */ ptr->type = ");
	printf("%d\n",ptr->type );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (ptr->type==ED_TYPE_ENTRY || ptr->type==ED_TYPE_DATA) {
tabout();
	printf("/*symbol address                */ ptr->address = ");
	printf("%d\n",ptr->address );
	}
	if (ptr->type==ED_TYPE_CONST) {
tabout();
	printf("/*constant value                */ ptr->constant = ");
	printf("%d\n",ptr->constant );
	}
tabout();
	printf("/*symbol name                   */ ptr->name = ");
	printf("%s\n",ptr->name );
}
void sysroff_print_sh_out (struct IT_sh *ptr)
{
itheader("sh", IT_sh_CODE);
tabout();
	printf("/*unit appearance number        */ ptr->unit = ");
	printf("%d\n",ptr->unit );
tabout();
	printf("/*section appearance number     */ ptr->section = ");
	printf("%d\n",ptr->section );
}
void sysroff_print_ob_out (struct IT_ob *ptr)
{
itheader("ob", IT_ob_CODE);
tabout();
	printf("/*starting address flag         */ ptr->saf = ");
	printf("%d\n",ptr->saf );
tabout();
	printf("/*compression flag              */ ptr->cpf = ");
	printf("%d\n",ptr->cpf );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (ptr->saf) {
tabout();
	printf("/*starting address              */ ptr->address = ");
	printf("%d\n",ptr->address );
	}
	if (ptr->cpf) {
tabout();
	printf("/*comp reps                     */ ptr->compreps = ");
	printf("%d\n",ptr->compreps );
	}
tabout();
	printf("/*data                          */ ptr->data = ");
	pbarray(&ptr->data );
}
void sysroff_print_rl_out (struct IT_rl *ptr)
{
itheader("rl", IT_rl_CODE);
tabout();
	printf("/*boundary of relocatable area  */ ptr->boundary = ");
	printf("%d\n",ptr->boundary );
tabout();
	printf("/*address polarity              */ ptr->apol = ");
	printf("%d\n",ptr->apol );
tabout();
	printf("/*segment number                */ ptr->segment = ");
	printf("%d\n",ptr->segment );
tabout();
	printf("/*sign of relocation            */ ptr->sign = ");
	printf("%d\n",ptr->sign );
tabout();
	printf("/*check range                   */ ptr->check = ");
	printf("%d\n",ptr->check );
tabout();
	printf("/*reloc address                 */ ptr->addr = ");
	printf("%d\n",ptr->addr );
tabout();
	printf("/*bit loc                       */ ptr->bitloc = ");
	printf("%d\n",ptr->bitloc );
tabout();
	printf("/*field length                  */ ptr->flen = ");
	printf("%d\n",ptr->flen );
tabout();
	printf("/*bcount                        */ ptr->bcount = ");
	printf("%d\n",ptr->bcount );
if (ptr->op  == 1) { tabout(); printf("OP_RELOC_ADDR\n");}
if (ptr->op  == 0) { tabout(); printf("OP_SEC_REF\n");}
if (ptr->op  == 2) { tabout(); printf("OP_EXT_REF\n");}
tabout();
	printf("/*operator                      */ ptr->op = ");
	printf("%d\n",ptr->op );
	if (ptr->op == OP_EXT_REF) {
tabout();
	printf("/*symbol number                 */ ptr->symn = ");
	printf("%d\n",ptr->symn );
	}
	if (ptr->op == OP_SEC_REF) {
tabout();
	printf("/*section number                */ ptr->secn = ");
	printf("%d\n",ptr->secn );
tabout();
	printf("/*const opcode                  */ ptr->copcode_is_3 = ");
	printf("%d\n",ptr->copcode_is_3 );
tabout();
	printf("/*addend length                 */ ptr->alength_is_4 = ");
	printf("%d\n",ptr->alength_is_4 );
tabout();
	printf("/*addend                        */ ptr->addend = ");
	printf("%d\n",ptr->addend );
tabout();
	printf("/*plus opcode                   */ ptr->aopcode_is_0x20 = ");
	printf("%d\n",ptr->aopcode_is_0x20 );
	}
	if (ptr->op == OP_RELOC_ADDR) {
tabout();
	printf("/*dunno                         */ ptr->dunno = ");
	printf("%d\n",ptr->dunno );
	}
tabout();
	printf("/*end                           */ ptr->end = ");
	printf("%d\n",ptr->end );
}
void sysroff_print_du_out (struct IT_du *ptr)
{
itheader("du", IT_du_CODE);
tabout();
	printf("/*format                        */ ptr->format = ");
	printf("%d\n",ptr->format );
tabout();
	printf("/*optimized                     */ ptr->optimized = ");
	printf("%d\n",ptr->optimized );
tabout();
	printf("/*stackfrmt                     */ ptr->stackfrmt = ");
	printf("%d\n",ptr->stackfrmt );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
tabout();
	printf("/*unit number                   */ ptr->unit = ");
	printf("%d\n",ptr->unit );
tabout();
	printf("/*sections                      */ ptr->sections = ");
	printf("%d\n",ptr->sections );
	printf("repeat %d\n", ptr->sections);
	{ int n; for (n = 0; n < ptr->sections; n++) {
tabout();
	printf("/*section appearance number     */ ptr->san = ");
	printf("%d\n",ptr->san[n]);
tabout();
	printf("/*address                       */ ptr->address = ");
	printf("%d\n",ptr->address[n]);
tabout();
	printf("/*section length                */ ptr->length = ");
	printf("%d\n",ptr->length[n]);
	}}
tabout();
	printf("/*tool name                     */ ptr->tool = ");
	printf("%s\n",ptr->tool );
tabout();
	printf("/*creation date                 */ ptr->date = ");
	printf("%s\n",ptr->date );
}
void sysroff_print_dsy_out (struct IT_dsy *ptr)
{
itheader("dsy", IT_dsy_CODE);
if (ptr->type  == 0) { tabout(); printf("STYPE_VAR\n");}
if (ptr->type  == 1) { tabout(); printf("STYPE_LAB\n");}
if (ptr->type  == 2) { tabout(); printf("STYPE_PROC\n");}
if (ptr->type  == 3) { tabout(); printf("STYPE_FUNC\n");}
if (ptr->type  == 4) { tabout(); printf("STYPE_TYPE\n");}
if (ptr->type  == 5) { tabout(); printf("STYPE_CONST\n");}
if (ptr->type  == 6) { tabout(); printf("STYPE_ENTRY\n");}
if (ptr->type  == 7) { tabout(); printf("STYPE_MEMBER\n");}
if (ptr->type  == 8) { tabout(); printf("STYPE_ENUM\n");}
if (ptr->type  == 9) { tabout(); printf("STYPE_TAG\n");}
if (ptr->type  == 10) { tabout(); printf("STYPE_PACKAGE\n");}
if (ptr->type  == 11) { tabout(); printf("STYPE_GENERIC\n");}
if (ptr->type  == 12) { tabout(); printf("STYPE_TASK\n");}
if (ptr->type  == 13) { tabout(); printf("STYPE_EXCEPTION\n");}
if (ptr->type  == 14) { tabout(); printf("STYPE_PARAMETER\n");}
if (ptr->type  == 15) { tabout(); printf("STYPE_EQUATE\n");}
if (ptr->type  == 0x7f) { tabout(); printf("STYPE_UNSPEC\n");}
tabout();
	printf("/*symbol type                   */ ptr->type = ");
	printf("%d\n",ptr->type );
tabout();
	printf("/*assignment info               */ ptr->assign = ");
	printf("%d\n",ptr->assign );
tabout();
	printf("/*symbol id                     */ ptr->snumber = ");
	printf("%d\n",ptr->snumber );
tabout();
	printf("/*symbol name                   */ ptr->sname = ");
	printf("%s\n",ptr->sname );
tabout();
	printf("/*nesting level                 */ ptr->nesting = ");
	printf("%d\n",ptr->nesting );
	if (ptr->assign) {
if (ptr->ainfo  == 1) { tabout(); printf("AINFO_REG\n");}
if (ptr->ainfo  == 2) { tabout(); printf("AINFO_STATIC_EXT_DEF\n");}
if (ptr->ainfo  == 3) { tabout(); printf("AINFO_STATIC_EXT_REF\n");}
if (ptr->ainfo  == 4) { tabout(); printf("AINFO_STATIC_INT\n");}
if (ptr->ainfo  == 5) { tabout(); printf("AINFO_STATIC_COM\n");}
if (ptr->ainfo  == 6) { tabout(); printf("AINFO_AUTO\n");}
if (ptr->ainfo  == 7) { tabout(); printf("AINFO_CONST\n");}
if (ptr->ainfo  == 0xff) { tabout(); printf("AINFO_UNSPEC\n");}
tabout();
	printf("/*assignment type               */ ptr->ainfo = ");
	printf("%d\n",ptr->ainfo );
tabout();
	printf("/*data length                   */ ptr->dlength = ");
	printf("%d\n",ptr->dlength );
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
            || ptr->ainfo == AINFO_STATIC_INT
            || ptr->ainfo == AINFO_STATIC_COM) {
tabout();
	printf("/*section number                */ ptr->section = ");
	printf("%d\n",ptr->section );
	}
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
            || ptr->ainfo == AINFO_STATIC_INT
            || ptr->ainfo == AINFO_STATIC_COM
            || ptr->ainfo == AINFO_AUTO) {
tabout();
	printf("/*address                       */ ptr->address = ");
	printf("%d\n",ptr->address );
	}
	if (ptr->ainfo == AINFO_REG) {
tabout();
	printf("/*register name                 */ ptr->reg = ");
	printf("%s\n",ptr->reg );
	}
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
	    || ptr->ainfo == AINFO_STATIC_EXT_REF) {
tabout();
	printf("/*external name                 */ ptr->ename = ");
	printf("%s\n",ptr->ename );
	}
	if (ptr->ainfo == AINFO_CONST) {
tabout();
	printf("/*constant                      */ ptr->constant = ");
	printf("%s\n",ptr->constant );
	}
	}
	if (ptr->type == STYPE_MEMBER) {
tabout();
	printf("/*assignment unit               */ ptr->bitunit = ");
	printf("%d\n",ptr->bitunit );
tabout();
	printf("/*spare                         */ ptr->spare2 = ");
	printf("%d\n",ptr->spare2 );
tabout();
	printf("/*field length                  */ ptr->field_len = ");
	printf("%d\n",ptr->field_len );
tabout();
	printf("/*field offset                  */ ptr->field_off = ");
	printf("%d\n",ptr->field_off );
	if (ptr->bitunit) {
tabout();
	printf("/*bit offset                    */ ptr->field_bitoff = ");
	printf("%d\n",ptr->field_bitoff );
	}
	}
	if (ptr->type== STYPE_ENUM) {
tabout();
	printf("/*value length                  */ ptr->evallen = ");
	printf("%d\n",ptr->evallen );
tabout();
	printf("/*value                         */ ptr->evalue = ");
	printf("%d\n",ptr->evalue );
	}
	if (ptr->type == STYPE_CONST) {
tabout();
	printf("/*value                         */ ptr->cvalue = ");
	printf("%s\n",ptr->cvalue );
	}
	if (ptr->type == STYPE_EQUATE) {
tabout();
	printf("/*value length                  */ ptr->qvallen = ");
	printf("%d\n",ptr->qvallen );
tabout();
	printf("/*value                         */ ptr->qvalue = ");
	printf("%d\n",ptr->qvalue );
tabout();
	printf("/*basic type                    */ ptr->btype = ");
	printf("%d\n",ptr->btype );
tabout();
	printf("/*size information              */ ptr->sizeinfo = ");
	printf("%d\n",ptr->sizeinfo );
tabout();
	printf("/*sign                          */ ptr->sign = ");
	printf("%d\n",ptr->sign );
tabout();
	printf("/*floating point type           */ ptr->flt_type = ");
	printf("%d\n",ptr->flt_type );
	}
tabout();
	printf("/*source file number            */ ptr->sfn = ");
	printf("%d\n",ptr->sfn );
tabout();
	printf("/*source line number            */ ptr->sln = ");
	printf("%d\n",ptr->sln );
tabout();
	printf("/*negotiation number            */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	if (ptr->type == STYPE_TAG) {
tabout();
	printf("/*magic                         */ ptr->magic = ");
	printf("%d\n",ptr->magic );
	}
}
void sysroff_print_dul_out (struct IT_dul *ptr)
{
itheader("dul", IT_dul_CODE);
tabout();
	printf("/*max declaration type flag     */ ptr->max_variable = ");
	printf("%d\n",ptr->max_variable );
tabout();
	printf("/*max spare                     */ ptr->maxspare = ");
	printf("%d\n",ptr->maxspare );
	if (ptr->max_variable == 0) {
tabout();
	printf("/*maximum                       */ ptr->max = ");
	printf("%d\n",ptr->max );
tabout();
	printf("/*max mode                      */ ptr->maxmode = ");
	printf("%s\n",ptr->maxmode );
	}
tabout();
	printf("/*min declaration type flag     */ ptr->min_variable = ");
	printf("%d\n",ptr->min_variable );
tabout();
	printf("/*min spare                     */ ptr->minspare = ");
	printf("%d\n",ptr->minspare );
	if (ptr->min_variable == 0) {
tabout();
	printf("/*minimum                       */ ptr->min = ");
	printf("%d\n",ptr->min );
tabout();
	printf("/*min mode                      */ ptr->minmode = ");
	printf("%s\n",ptr->minmode );
	}
}
void sysroff_print_dty_out (struct IT_dty *ptr)
{
itheader("dty", IT_dty_CODE);
tabout();
	printf("/*end flag                      */ ptr->end = ");
	printf("%d\n",ptr->end );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (!ptr->end) {
tabout();
	printf("/*negotiation                   */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	}
}
void sysroff_print_dbt_out (struct IT_dbt *ptr)
{
itheader("dbt", IT_dbt_CODE);
if (ptr->btype  == 0) { tabout(); printf("BTYPE_VOID\n");}
if (ptr->btype  == 1) { tabout(); printf("BTYPE_UNDEF\n");}
if (ptr->btype  == 2) { tabout(); printf("BTYPE_CHAR\n");}
if (ptr->btype  == 3) { tabout(); printf("BTYPE_INT\n");}
if (ptr->btype  == 4) { tabout(); printf("BTYPE_FLOAT\n");}
if (ptr->btype  == 5) { tabout(); printf("BTYPE_BIT\n");}
if (ptr->btype  == 6) { tabout(); printf("BTYPE_STRING\n");}
if (ptr->btype  == 7) { tabout(); printf("BTYPE_DECIMAL\n");}
if (ptr->btype  == 8) { tabout(); printf("BTYPE_ENUM\n");}
if (ptr->btype  == 9) { tabout(); printf("BTYPE_STRUCT\n");}
if (ptr->btype  == 10) { tabout(); printf("BTYPE_TYPE\n");}
if (ptr->btype  == 11) { tabout(); printf("BTYPE_TAG\n");}
if (ptr->btype  == 0xff) { tabout(); printf("BTYPE_UNSPEC\n");}
tabout();
	printf("/*basic type                    */ ptr->btype = ");
	printf("%d\n",ptr->btype );
tabout();
	printf("/*size info                     */ ptr->bitsize = ");
	printf("%d\n",ptr->bitsize );
if (ptr->sign  == 0) { tabout(); printf("SIGN_SIGNED\n");}
if (ptr->sign  == 1) { tabout(); printf("SIGN_UNSIGNED\n");}
if (ptr->sign  == 3) { tabout(); printf("SIGN_UNSPEC\n");}
tabout();
	printf("/*sign                          */ ptr->sign = ");
	printf("%d\n",ptr->sign );
if (ptr->fptype  == 0) { tabout(); printf("FPTYPE_SINGLE\n");}
if (ptr->fptype  == 1) { tabout(); printf("FPTYPE_DOUBLE\n");}
if (ptr->fptype  == 2) { tabout(); printf("FPTYPE_EXTENDED\n");}
if (ptr->fptype  == 0x3f) { tabout(); printf("FPTYPE_NOTSPEC\n");}
tabout();
	printf("/*floating point type           */ ptr->fptype = ");
	printf("%d\n",ptr->fptype );
	if (ptr->btype==BTYPE_TAG || ptr->btype == BTYPE_TYPE) {
tabout();
	printf("/*symbol id                     */ ptr->sid = ");
	printf("%d\n",ptr->sid );
	}
tabout();
	printf("/*negotiation                   */ ptr->neg = ");
	printf("%d\n",ptr->neg );
}
void sysroff_print_dar_out (struct IT_dar *ptr)
{
itheader("dar", IT_dar_CODE);
tabout();
	printf("/*element length                */ ptr->length = ");
	printf("%d\n",ptr->length );
tabout();
	printf("/*dims                          */ ptr->dims = ");
	printf("%d\n",ptr->dims );
	printf("repeat %d\n", ptr->dims);
	{ int n; for (n = 0; n < ptr->dims; n++) {
if (ptr->variable[n] == 0) { tabout(); printf("VARIABLE_FIXED\n");}
if (ptr->variable[n] == 1) { tabout(); printf("VARIABLE_VARIABLE\n");}
tabout();
	printf("/*variable flag                 */ ptr->variable = ");
	printf("%d\n",ptr->variable[n]);
if (ptr->subtype[n] == 0) { tabout(); printf("SUB_INTEGER\n");}
if (ptr->subtype[n] == 1) { tabout(); printf("SUB_TYPE\n");}
tabout();
	printf("/*subscript type                */ ptr->subtype = ");
	printf("%d\n",ptr->subtype[n]);
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare[n]);
	if (ptr->subtype[n] == SUB_TYPE) {
tabout();
	printf("/*sub symbol id                 */ ptr->sid = ");
	printf("%d\n",ptr->sid[n]);
	}
	if (ptr->subtype[n] == SUB_INTEGER) {
tabout();
	printf("/*max declaration type flag     */ ptr->max_variable = ");
	printf("%d\n",ptr->max_variable[n]);
tabout();
	printf("/*max spare                     */ ptr->maxspare = ");
	printf("%d\n",ptr->maxspare[n]);
tabout();
	printf("/*maximum                       */ ptr->max = ");
	printf("%d\n",ptr->max[n]);
tabout();
	printf("/*min declaration type flag     */ ptr->min_variable = ");
	printf("%d\n",ptr->min_variable[n]);
tabout();
	printf("/*min spare                     */ ptr->minspare = ");
	printf("%d\n",ptr->minspare[n]);
tabout();
	printf("/*minimum                       */ ptr->min = ");
	printf("%d\n",ptr->min[n]);
	}
	}}
tabout();
	printf("/*negotiation                   */ ptr->neg = ");
	printf("%d\n",ptr->neg );
}
void sysroff_print_dso_out (struct IT_dso *ptr)
{
itheader("dso", IT_dso_CODE);
tabout();
	printf("/*function name                 */ ptr->sid = ");
	printf("%d\n",ptr->sid );
tabout();
	printf("/*sp update count               */ ptr->spupdates = ");
	printf("%d\n",ptr->spupdates );
	printf("repeat %d\n", ptr->spupdates);
	{ int n; for (n = 0; n < ptr->spupdates; n++) {
tabout();
	printf("/*update address                */ ptr->address = ");
	printf("%d\n",ptr->address[n]);
tabout();
	printf("/*offset                        */ ptr->offset = ");
	printf("%d\n",ptr->offset[n]);
	}}
}
void sysroff_print_dln_out (struct IT_dln *ptr)
{
itheader("dln", IT_dln_CODE);
tabout();
	printf("/*number of lines               */ ptr->nln = ");
	printf("%d\n",ptr->nln );
	printf("repeat %d\n", ptr->nln);
	{ int n; for (n = 0; n < ptr->nln; n++) {
tabout();
	printf("/*source file number            */ ptr->sfn = ");
	printf("%d\n",ptr->sfn[n]);
tabout();
	printf("/*source line number            */ ptr->sln = ");
	printf("%d\n",ptr->sln[n]);
tabout();
	printf("/*section number                */ ptr->section = ");
	printf("%d\n",ptr->section[n]);
tabout();
	printf("/*from address                  */ ptr->from_address = ");
	printf("%d\n",ptr->from_address[n]);
tabout();
	printf("/*to address                    */ ptr->to_address = ");
	printf("%d\n",ptr->to_address[n]);
tabout();
	printf("/*call count                    */ ptr->cc = ");
	printf("%d\n",ptr->cc[n]);
	}}
tabout();
	printf("/*neg                           */ ptr->neg = ");
	printf("%d\n",ptr->neg );
}
void sysroff_print_dpp_out (struct IT_dpp *ptr)
{
itheader("dpp", IT_dpp_CODE);
tabout();
	printf("/*start/end                     */ ptr->end = ");
	printf("%d\n",ptr->end );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (!ptr->end) {
tabout();
	printf("/*params                        */ ptr->params = ");
	printf("%d\n",ptr->params );
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	}
}
void sysroff_print_den_out (struct IT_den *ptr)
{
itheader("den", IT_den_CODE);
tabout();
	printf("/*start/end                     */ ptr->end = ");
	printf("%d\n",ptr->end );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (!ptr->end) {
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	}
}
void sysroff_print_dfp_out (struct IT_dfp *ptr)
{
itheader("dfp", IT_dfp_CODE);
tabout();
	printf("/*start/end flag                */ ptr->end = ");
	printf("%d\n",ptr->end );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (!ptr->end) {
tabout();
	printf("/*number of parameters          */ ptr->nparams = ");
	printf("%d\n",ptr->nparams );
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	}
}
void sysroff_print_dds_out (struct IT_dds *ptr)
{
itheader("dds", IT_dds_CODE);
tabout();
	printf("/*start/end                     */ ptr->end = ");
	printf("%d\n",ptr->end );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (!ptr->end) {
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	}
}
void sysroff_print_dpt_out (struct IT_dpt *ptr)
{
itheader("dpt", IT_dpt_CODE);
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
tabout();
	printf("/*dunno                         */ ptr->dunno = ");
	printf("%d\n",ptr->dunno );
}
void sysroff_print_dse_out (struct IT_dse *ptr)
{
itheader("dse", IT_dse_CODE);
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
tabout();
	printf("/*dunno                         */ ptr->dunno = ");
	printf("%d\n",ptr->dunno );
}
void sysroff_print_dot_out (struct IT_dot *ptr)
{
itheader("dot", IT_dot_CODE);
tabout();
	printf("/*unknown                       */ ptr->unknown = ");
	printf("%d\n",ptr->unknown );
}
void sysroff_print_dss_out (struct IT_dss *ptr)
{
itheader("dss", IT_dss_CODE);
tabout();
	printf("/*type                          */ ptr->type = ");
	printf("%d\n",ptr->type );
tabout();
	printf("/*external/internal             */ ptr->internal = ");
	printf("%d\n",ptr->internal );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	if (!ptr->internal) {
tabout();
	printf("/*package name                  */ ptr->package = ");
	printf("%s\n",ptr->package );
	}
	if (ptr->internal) {
tabout();
	printf("/*symbol id                     */ ptr->id = ");
	printf("%d\n",ptr->id );
	}
tabout();
	printf("/*record type                   */ ptr->record = ");
	printf("%d\n",ptr->record );
tabout();
	printf("/*rules                         */ ptr->rules = ");
	printf("%s\n",ptr->rules );
tabout();
	printf("/*number of symbols             */ ptr->nsymbols = ");
	printf("%d\n",ptr->nsymbols );
tabout();
	printf("/*unknown                       */ ptr->fixme = ");
	printf("%d\n",ptr->fixme );
}
void sysroff_print_pss_out (struct IT_pss *ptr)
{
itheader("pss", IT_pss_CODE);
tabout();
	printf("/*negotiation number            */ ptr->efn = ");
	printf("%d\n",ptr->efn );
tabout();
	printf("/*number of source files        */ ptr->ns = ");
	printf("%d\n",ptr->ns );
	printf("repeat %d\n", ptr->ns);
	{ int n; for (n = 0; n < ptr->ns; n++) {
tabout();
	printf("/*directory reference bit       */ ptr->drb = ");
	printf("%d\n",ptr->drb[n]);
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare[n]);
tabout();
	printf("/*completed file name           */ ptr->fname = ");
	printf("%s\n",ptr->fname[n]);
	if (ptr->drb[n]) {
tabout();
	printf("/*directory apperance number    */ ptr->dan = ");
	printf("%d\n",ptr->dan[n]);
	}
	}}
tabout();
	printf("/*number of directories         */ ptr->ndir = ");
	printf("%d\n",ptr->ndir );
	printf("repeat %d\n", ptr->ndir);
	{ int n; for (n = 0; n < ptr->ndir; n++) {
tabout();
	printf("/*directory name                */ ptr->dname = ");
	printf("%s\n",ptr->dname[n]);
	}}
}
void sysroff_print_dus_out (struct IT_dus *ptr)
{
itheader("dus", IT_dus_CODE);
tabout();
	printf("/*negotiation number            */ ptr->efn = ");
	printf("%d\n",ptr->efn );
tabout();
	printf("/*number of source files        */ ptr->ns = ");
	printf("%d\n",ptr->ns );
	printf("repeat %d\n", ptr->ns);
	{ int n; for (n = 0; n < ptr->ns; n++) {
tabout();
	printf("/*directory reference bit       */ ptr->drb = ");
	printf("%d\n",ptr->drb[n]);
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare[n]);
tabout();
	printf("/*completed file name           */ ptr->fname = ");
	printf("%s\n",ptr->fname[n]);
	if (ptr->drb[n]) {
tabout();
	printf("/*directory apperance number    */ ptr->dan = ");
	printf("%d\n",ptr->dan[n]);
	}
	}}
tabout();
	printf("/*number of directories         */ ptr->ndir = ");
	printf("%d\n",ptr->ndir );
	printf("repeat %d\n", ptr->ndir);
	{ int n; for (n = 0; n < ptr->ndir; n++) {
tabout();
	printf("/*directory name                */ ptr->dname = ");
	printf("%s\n",ptr->dname[n]);
	}}
}
void sysroff_print_dps_out (struct IT_dps *ptr)
{
itheader("dps", IT_dps_CODE);
tabout();
	printf("/*start/end flag                */ ptr->end = ");
	printf("%d\n",ptr->end );
if (ptr->type  == 0) { tabout(); printf("BLOCK_TYPE_COMPUNIT\n");}
if (ptr->type  == 2) { tabout(); printf("BLOCK_TYPE_PROCEDURE\n");}
if (ptr->type  == 3) { tabout(); printf("BLOCK_TYPE_FUNCTION\n");}
if (ptr->type  == 4) { tabout(); printf("BLOCK_TYPE_BLOCK\n");}
if (ptr->type  == 9) { tabout(); printf("BLOCK_TYPE_BASIC\n");}
tabout();
	printf("/*block type                    */ ptr->type = ");
	printf("%d\n",ptr->type );
	if (!ptr->end) {
tabout();
	printf("/*optimization                  */ ptr->opt = ");
	printf("%d\n",ptr->opt );
tabout();
	printf("/*section number                */ ptr->san = ");
	printf("%d\n",ptr->san );
tabout();
	printf("/*address                       */ ptr->address = ");
	printf("%d\n",ptr->address );
tabout();
	printf("/*block size                    */ ptr->block_size = ");
	printf("%d\n",ptr->block_size );
tabout();
	printf("/*nesting                       */ ptr->nesting = ");
	printf("%d\n",ptr->nesting );
	if (ptr->type == BLOCK_TYPE_PROCEDURE
	    || ptr->type == BLOCK_TYPE_FUNCTION) {
tabout();
	printf("/*return address                */ ptr->retaddr = ");
	printf("%d\n",ptr->retaddr );
tabout();
	printf("/*interrupt function flag       */ ptr->intrflag = ");
	printf("%d\n",ptr->intrflag );
tabout();
	printf("/*stack update flag             */ ptr->stackflag = ");
	printf("%d\n",ptr->stackflag );
tabout();
	printf("/*intra page JMP                */ ptr->intrpagejmp = ");
	printf("%d\n",ptr->intrpagejmp );
tabout();
	printf("/*spare                         */ ptr->spare = ");
	printf("%d\n",ptr->spare );
	}
tabout();
	printf("/*neg number                    */ ptr->neg = ");
	printf("%d\n",ptr->neg );
	}
}
#endif
#ifdef SYSROFF_SWAP_IN
void sysroff_swap_cs_in (struct IT_cs * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->size  = getINT(raw,&idx, 1,size);
	ptr->hd  = getINT(raw,&idx, 1,size);
	ptr->hs  = getINT(raw,&idx, 1,size);
	ptr->un  = getINT(raw,&idx, 1,size);
	ptr->us  = getINT(raw,&idx, 1,size);
	ptr->sc  = getINT(raw,&idx, 1,size);
	ptr->ss  = getINT(raw,&idx, 1,size);
	ptr->er  = getINT(raw,&idx, 1,size);
	ptr->ed  = getINT(raw,&idx, 1,size);
	ptr->sh  = getINT(raw,&idx, 1,size);
	ptr->ob  = getINT(raw,&idx, 1,size);
	ptr->rl  = getINT(raw,&idx, 1,size);
	ptr->du  = getINT(raw,&idx, 1,size);
	ptr->dps  = getINT(raw,&idx, 1,size);
	ptr->dsy  = getINT(raw,&idx, 1,size);
	ptr->dty  = getINT(raw,&idx, 1,size);
	ptr->dln  = getINT(raw,&idx, 1,size);
	ptr->dso  = getINT(raw,&idx, 1,size);
	ptr->dus  = getINT(raw,&idx, 1,size);
	ptr->dss  = getINT(raw,&idx, 1,size);
	ptr->dbt  = getINT(raw,&idx, 1,size);
	ptr->dpp  = getINT(raw,&idx, 1,size);
	ptr->dfp  = getINT(raw,&idx, 1,size);
	ptr->den  = getINT(raw,&idx, 1,size);
	ptr->dds  = getINT(raw,&idx, 1,size);
	ptr->dar  = getINT(raw,&idx, 1,size);
	ptr->dpt  = getINT(raw,&idx, 1,size);
	ptr->dul  = getINT(raw,&idx, 1,size);
	ptr->dse  = getINT(raw,&idx, 1,size);
	ptr->dot  = getINT(raw,&idx, 1,size);
}
void sysroff_swap_hd_in (struct IT_hd * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->mt  = getBITS(raw,&idx, 4,size);
	ptr->spare1  = getBITS(raw,&idx, 4,size);
	ptr->cd  = getCHARS(raw,&idx, 12,size);
	ptr->nu  = getINT(raw,&idx, 2,size);
	ptr->code  = getINT(raw,&idx, 1,size);
	ptr->ver  = getCHARS(raw,&idx, 4,size);
	ptr->au  = getINT(raw,&idx, 1,size);
	ptr->si  = getBITS(raw,&idx, 1,size);
	ptr->afl  = getBITS(raw,&idx, 4,size);
	ptr->spare2  = getBITS(raw,&idx, 3,size);
	ptr->spcsz  = getINT(raw,&idx, 1,size);
	ptr->segsz  = getINT(raw,&idx, 1,size);
	ptr->segsh  = getINT(raw,&idx, 1,size);
	ptr->ep  = getINT(raw,&idx, 1,size);
	if (ptr->ep) {
	if (ptr->mt != MTYPE_ABS_LM) {
	ptr->uan  = getINT(raw,&idx, 2,size);
	ptr->sa  = getINT(raw,&idx, 2,size);
	}
	if (segmented_p) {
	ptr->sad  = getINT(raw,&idx, -1,size);
	}
	ptr->address  = getINT(raw,&idx, -2,size);
	}
	ptr->os  = getCHARS(raw,&idx, 0,size);
	ptr->sys  = getCHARS(raw,&idx, 0,size);
	ptr->mn  = getCHARS(raw,&idx, 0,size);
	ptr->cpu  = getCHARS(raw,&idx, 0,size);
}
void sysroff_swap_hs_in (struct IT_hs * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->neg  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_un_in (struct IT_un * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->format  = getBITS(raw,&idx, 2,size);
	ptr->spare1  = getBITS(raw,&idx, 6,size);
	ptr->nsections  = getINT(raw,&idx, 2,size);
	ptr->nextrefs  = getINT(raw,&idx, 2,size);
	ptr->nextdefs  = getINT(raw,&idx, 2,size);
	ptr->name  = getCHARS(raw,&idx, 0,size);
	ptr->tool  = getCHARS(raw,&idx, 0,size);
	ptr->tcd  = getCHARS(raw,&idx, 12,size);
	ptr->linker  = getCHARS(raw,&idx, 0,size);
	ptr->lcd  = getCHARS(raw,&idx, 12,size);
}
void sysroff_swap_us_in (struct IT_us * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->neg  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_sc_in (struct IT_sc * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->format  = getBITS(raw,&idx, 2,size);
	ptr->spare  = getBITS(raw,&idx, 6,size);
	ptr->segadd  = getINT(raw,&idx, -1,size);
	ptr->addr  = getINT(raw,&idx, -2,size);
	ptr->length  = getINT(raw,&idx, -2,size);
	ptr->align  = getINT(raw,&idx, -2,size);
	ptr->contents  = getBITS(raw,&idx, 4,size);
	ptr->concat  = getBITS(raw,&idx, 4,size);
	ptr->read  = getBITS(raw,&idx, 2,size);
	ptr->write  = getBITS(raw,&idx, 2,size);
	ptr->exec  = getBITS(raw,&idx, 2,size);
	ptr->init  = getBITS(raw,&idx, 2,size);
	ptr->mode  = getBITS(raw,&idx, 2,size);
	ptr->spare1  = getBITS(raw,&idx, 6,size);
	ptr->name  = getCHARS(raw,&idx, 0,size);
}
void sysroff_swap_ss_in (struct IT_ss * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->neg  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_er_in (struct IT_er * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->type  = getBITS(raw,&idx, 2,size);
	ptr->spare  = getBITS(raw,&idx, 6,size);
	ptr->name  = getCHARS(raw,&idx, 0,size);
}
void sysroff_swap_ed_in (struct IT_ed * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->section  = getINT(raw,&idx, 2,size);
	ptr->type  = getBITS(raw,&idx, 3,size);
	ptr->spare  = getBITS(raw,&idx, 5,size);
	if (ptr->type==ED_TYPE_ENTRY || ptr->type==ED_TYPE_DATA) {
	ptr->address  = getINT(raw,&idx, -2,size);
	}
	if (ptr->type==ED_TYPE_CONST) {
	ptr->constant  = getINT(raw,&idx, -2,size);
	}
	ptr->name  = getCHARS(raw,&idx, 0,size);
}
void sysroff_swap_sh_in (struct IT_sh * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->unit  = getINT(raw,&idx, 2,size);
	ptr->section  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_ob_in (struct IT_ob * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->saf  = getBITS(raw,&idx, 1,size);
	ptr->cpf  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 6,size);
	if (ptr->saf) {
	ptr->address  = getINT(raw,&idx, -2,size);
	}
	if (ptr->cpf) {
	ptr->compreps  = getINT(raw,&idx, -2,size);
	}
	ptr->data  = getBARRAY(raw,&idx, -4,size);
}
void sysroff_swap_rl_in (struct IT_rl * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->boundary  = getBITS(raw,&idx, 4,size);
	ptr->apol  = getBITS(raw,&idx, 1,size);
	ptr->segment  = getBITS(raw,&idx, 1,size);
	ptr->sign  = getBITS(raw,&idx, 1,size);
	ptr->check  = getBITS(raw,&idx, 1,size);
	ptr->addr  = getINT(raw,&idx, -2,size);
	ptr->bitloc  = getINT(raw,&idx, 1,size);
	ptr->flen  = getINT(raw,&idx, 1,size);
	ptr->bcount  = getINT(raw,&idx, 1,size);
	ptr->op  = getINT(raw,&idx, 1,size);
	if (ptr->op == OP_EXT_REF) {
	ptr->symn  = getINT(raw,&idx, 2,size);
	}
	if (ptr->op == OP_SEC_REF) {
	ptr->secn  = getINT(raw,&idx, 2,size);
	ptr->copcode_is_3  = getINT(raw,&idx, 1,size);
	ptr->alength_is_4  = getINT(raw,&idx, 1,size);
	ptr->addend  = getINT(raw,&idx, 4,size);
	ptr->aopcode_is_0x20  = getINT(raw,&idx, 1,size);
	}
	if (ptr->op == OP_RELOC_ADDR) {
	ptr->dunno  = getINT(raw,&idx, 2,size);
	}
	ptr->end  = getINT(raw,&idx, 1,size);
}
void sysroff_swap_du_in (struct IT_du * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->format  = getBITS(raw,&idx, 2,size);
	ptr->optimized  = getBITS(raw,&idx, 1,size);
	ptr->stackfrmt  = getBITS(raw,&idx, 2,size);
	ptr->spare  = getBITS(raw,&idx, 3,size);
	ptr->unit  = getINT(raw,&idx, 2,size);
	ptr->sections  = getINT(raw,&idx, 2,size);
	{ int n; for (n = 0; n < ptr->sections; n++) {
if (!ptr->san) ptr->san = (INT*)xcalloc(ptr->sections, sizeof(ptr->san[0]));
	ptr->san[n] = getINT(raw,&idx, 2,size);
if (!ptr->address) ptr->address = (INT*)xcalloc(ptr->sections, sizeof(ptr->address[0]));
	ptr->address[n] = getINT(raw,&idx, -2,size);
if (!ptr->length) ptr->length = (INT*)xcalloc(ptr->sections, sizeof(ptr->length[0]));
	ptr->length[n] = getINT(raw,&idx, -2,size);
	}}
	ptr->tool  = getCHARS(raw,&idx, 0,size);
	ptr->date  = getCHARS(raw,&idx, 12,size);
}
void sysroff_swap_dsy_in (struct IT_dsy * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->type  = getBITS(raw,&idx, 7,size);
	ptr->assign  = getBITS(raw,&idx, 1,size);
	ptr->snumber  = getINT(raw,&idx, 2,size);
	ptr->sname  = getCHARS(raw,&idx, 0,size);
	ptr->nesting  = getINT(raw,&idx, 2,size);
	if (ptr->assign) {
	ptr->ainfo  = getINT(raw,&idx, 1,size);
	ptr->dlength  = getINT(raw,&idx, -2,size);
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
            || ptr->ainfo == AINFO_STATIC_INT
            || ptr->ainfo == AINFO_STATIC_COM) {
	ptr->section  = getINT(raw,&idx, 2,size);
	}
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
            || ptr->ainfo == AINFO_STATIC_INT
            || ptr->ainfo == AINFO_STATIC_COM
            || ptr->ainfo == AINFO_AUTO) {
	ptr->address  = getINT(raw,&idx, -2,size);
	}
	if (ptr->ainfo == AINFO_REG) {
	ptr->reg  = getCHARS(raw,&idx, 0,size);
	}
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
	    || ptr->ainfo == AINFO_STATIC_EXT_REF) {
	ptr->ename  = getCHARS(raw,&idx, 0,size);
	}
	if (ptr->ainfo == AINFO_CONST) {
	ptr->constant  = getCHARS(raw,&idx, 0,size);
	}
	}
	if (ptr->type == STYPE_MEMBER) {
	ptr->bitunit  = getBITS(raw,&idx, 1,size);
	ptr->spare2  = getBITS(raw,&idx, 7,size);
	ptr->field_len  = getINT(raw,&idx, -2,size);
	ptr->field_off  = getINT(raw,&idx, -2,size);
	if (ptr->bitunit) {
	ptr->field_bitoff  = getINT(raw,&idx, -2,size);
	}
	}
	if (ptr->type== STYPE_ENUM) {
	ptr->evallen  = getINT(raw,&idx, 1,size);
	ptr->evalue  = getINT(raw,&idx, 4,size);
	}
	if (ptr->type == STYPE_CONST) {
	ptr->cvalue  = getCHARS(raw,&idx, 0,size);
	}
	if (ptr->type == STYPE_EQUATE) {
	ptr->qvallen  = getINT(raw,&idx, 1,size);
	ptr->qvalue  = getINT(raw,&idx, 4,size);
	ptr->btype  = getINT(raw,&idx, 1,size);
	ptr->sizeinfo  = getINT(raw,&idx, -2,size);
	ptr->sign  = getBITS(raw,&idx, 2,size);
	ptr->flt_type  = getBITS(raw,&idx, 6,size);
	}
	ptr->sfn  = getINT(raw,&idx, 2,size);
	ptr->sln  = getINT(raw,&idx, 2,size);
	ptr->neg  = getINT(raw,&idx, 2,size);
	if (ptr->type == STYPE_TAG) {
	ptr->magic  = getINT(raw,&idx, 1,size);
	}
}
void sysroff_swap_dul_in (struct IT_dul * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->max_variable  = getBITS(raw,&idx, 1,size);
	ptr->maxspare  = getBITS(raw,&idx, 7,size);
	if (ptr->max_variable == 0) {
	ptr->max  = getINT(raw,&idx, -2,size);
	ptr->maxmode  = getCHARS(raw,&idx, 0,size);
	}
	ptr->min_variable  = getBITS(raw,&idx, 1,size);
	ptr->minspare  = getBITS(raw,&idx, 7,size);
	if (ptr->min_variable == 0) {
	ptr->min  = getINT(raw,&idx, -2,size);
	ptr->minmode  = getCHARS(raw,&idx, 0,size);
	}
}
void sysroff_swap_dty_in (struct IT_dty * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->end  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 7,size);
	if (!ptr->end) {
	ptr->neg  = getINT(raw,&idx, 2,size);
	}
}
void sysroff_swap_dbt_in (struct IT_dbt * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->btype  = getINT(raw,&idx, 1,size);
	ptr->bitsize  = getINT(raw,&idx, -2,size);
	ptr->sign  = getBITS(raw,&idx, 2,size);
	ptr->fptype  = getBITS(raw,&idx, 6,size);
	if (ptr->btype==BTYPE_TAG || ptr->btype == BTYPE_TYPE) {
	ptr->sid  = getINT(raw,&idx, 2,size);
	}
	ptr->neg  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_dar_in (struct IT_dar * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->length  = getINT(raw,&idx, -2,size);
	ptr->dims  = getINT(raw,&idx, 1,size);
	{ int n; for (n = 0; n < ptr->dims; n++) {
if (!ptr->variable) ptr->variable = (INT*)xcalloc(ptr->dims, sizeof(ptr->variable[0]));
	ptr->variable[n] = getBITS(raw,&idx, 1,size);
if (!ptr->subtype) ptr->subtype = (INT*)xcalloc(ptr->dims, sizeof(ptr->subtype[0]));
	ptr->subtype[n] = getBITS(raw,&idx, 1,size);
if (!ptr->spare) ptr->spare = (INT*)xcalloc(ptr->dims, sizeof(ptr->spare[0]));
	ptr->spare[n] = getBITS(raw,&idx, 6,size);
	if (ptr->subtype[n] == SUB_TYPE) {
if (!ptr->sid) ptr->sid = (INT*)xcalloc(ptr->dims, sizeof(ptr->sid[0]));
	ptr->sid[n] = getINT(raw,&idx, 2,size);
	}
	if (ptr->subtype[n] == SUB_INTEGER) {
if (!ptr->max_variable) ptr->max_variable = (INT*)xcalloc(ptr->dims, sizeof(ptr->max_variable[0]));
	ptr->max_variable[n] = getBITS(raw,&idx, 1,size);
if (!ptr->maxspare) ptr->maxspare = (INT*)xcalloc(ptr->dims, sizeof(ptr->maxspare[0]));
	ptr->maxspare[n] = getBITS(raw,&idx, 7,size);
if (!ptr->max) ptr->max = (INT*)xcalloc(ptr->dims, sizeof(ptr->max[0]));
	ptr->max[n] = getINT(raw,&idx, -2,size);
if (!ptr->min_variable) ptr->min_variable = (INT*)xcalloc(ptr->dims, sizeof(ptr->min_variable[0]));
	ptr->min_variable[n] = getBITS(raw,&idx, 1,size);
if (!ptr->minspare) ptr->minspare = (INT*)xcalloc(ptr->dims, sizeof(ptr->minspare[0]));
	ptr->minspare[n] = getBITS(raw,&idx, 7,size);
if (!ptr->min) ptr->min = (INT*)xcalloc(ptr->dims, sizeof(ptr->min[0]));
	ptr->min[n] = getINT(raw,&idx, -2,size);
	}
	}}
	ptr->neg  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_dso_in (struct IT_dso * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->sid  = getINT(raw,&idx, 2,size);
	ptr->spupdates  = getINT(raw,&idx, 4,size);
	{ int n; for (n = 0; n < ptr->spupdates; n++) {
if (!ptr->address) ptr->address = (INT*)xcalloc(ptr->spupdates, sizeof(ptr->address[0]));
	ptr->address[n] = getINT(raw,&idx, -2,size);
if (!ptr->offset) ptr->offset = (INT*)xcalloc(ptr->spupdates, sizeof(ptr->offset[0]));
	ptr->offset[n] = getINT(raw,&idx, -2,size);
	}}
}
void sysroff_swap_dln_in (struct IT_dln * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->nln  = getINT(raw,&idx, 2,size);
	{ int n; for (n = 0; n < ptr->nln; n++) {
if (!ptr->sfn) ptr->sfn = (INT*)xcalloc(ptr->nln, sizeof(ptr->sfn[0]));
	ptr->sfn[n] = getINT(raw,&idx, 2,size);
if (!ptr->sln) ptr->sln = (INT*)xcalloc(ptr->nln, sizeof(ptr->sln[0]));
	ptr->sln[n] = getINT(raw,&idx, 2,size);
if (!ptr->section) ptr->section = (INT*)xcalloc(ptr->nln, sizeof(ptr->section[0]));
	ptr->section[n] = getINT(raw,&idx, 2,size);
if (!ptr->from_address) ptr->from_address = (INT*)xcalloc(ptr->nln, sizeof(ptr->from_address[0]));
	ptr->from_address[n] = getINT(raw,&idx, -2,size);
if (!ptr->to_address) ptr->to_address = (INT*)xcalloc(ptr->nln, sizeof(ptr->to_address[0]));
	ptr->to_address[n] = getINT(raw,&idx, -2,size);
if (!ptr->cc) ptr->cc = (INT*)xcalloc(ptr->nln, sizeof(ptr->cc[0]));
	ptr->cc[n] = getINT(raw,&idx, 2,size);
	}}
	ptr->neg  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_dpp_in (struct IT_dpp * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->end  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 7,size);
	if (!ptr->end) {
	ptr->params  = getINT(raw,&idx, 1,size);
	ptr->neg  = getINT(raw,&idx, 2,size);
	}
}
void sysroff_swap_den_in (struct IT_den * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->end  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 7,size);
	if (!ptr->end) {
	ptr->neg  = getINT(raw,&idx, 2,size);
	}
}
void sysroff_swap_dfp_in (struct IT_dfp * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->end  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 7,size);
	if (!ptr->end) {
	ptr->nparams  = getINT(raw,&idx, 1,size);
	ptr->neg  = getINT(raw,&idx, 2,size);
	}
}
void sysroff_swap_dds_in (struct IT_dds * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->end  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 7,size);
	if (!ptr->end) {
	ptr->neg  = getINT(raw,&idx, 2,size);
	}
}
void sysroff_swap_dpt_in (struct IT_dpt * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->neg  = getINT(raw,&idx, 2,size);
	ptr->dunno  = getINT(raw,&idx, 1,size);
}
void sysroff_swap_dse_in (struct IT_dse * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->neg  = getINT(raw,&idx, 2,size);
	ptr->dunno  = getINT(raw,&idx, 1,size);
}
void sysroff_swap_dot_in (struct IT_dot * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->unknown  = getINT(raw,&idx, 1,size);
}
void sysroff_swap_dss_in (struct IT_dss * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->type  = getINT(raw,&idx, 1,size);
	ptr->internal  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 7,size);
	if (!ptr->internal) {
	ptr->package  = getCHARS(raw,&idx, 0,size);
	}
	if (ptr->internal) {
	ptr->id  = getINT(raw,&idx, 2,size);
	}
	ptr->record  = getINT(raw,&idx, 2,size);
	ptr->rules  = getCHARS(raw,&idx, 0,size);
	ptr->nsymbols  = getINT(raw,&idx, 2,size);
	ptr->fixme  = getINT(raw,&idx, 2,size);
}
void sysroff_swap_pss_in (struct IT_pss * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->efn  = getINT(raw,&idx, 2,size);
	ptr->ns  = getINT(raw,&idx, 2,size);
	{ int n; for (n = 0; n < ptr->ns; n++) {
if (!ptr->drb) ptr->drb = (INT*)xcalloc(ptr->ns, sizeof(ptr->drb[0]));
	ptr->drb[n] = getBITS(raw,&idx, 1,size);
if (!ptr->spare) ptr->spare = (INT*)xcalloc(ptr->ns, sizeof(ptr->spare[0]));
	ptr->spare[n] = getBITS(raw,&idx, 7,size);
if (!ptr->fname) ptr->fname = (CHARS*)xcalloc(ptr->ns, sizeof(ptr->fname[0]));
	ptr->fname[n] = getCHARS(raw,&idx, 0,size);
	if (ptr->drb[n]) {
if (!ptr->dan) ptr->dan = (INT*)xcalloc(ptr->ns, sizeof(ptr->dan[0]));
	ptr->dan[n] = getINT(raw,&idx, 2,size);
	}
	}}
	ptr->ndir  = getINT(raw,&idx, 2,size);
	{ int n; for (n = 0; n < ptr->ndir; n++) {
if (!ptr->dname) ptr->dname = (CHARS*)xcalloc(ptr->ndir, sizeof(ptr->dname[0]));
	ptr->dname[n] = getCHARS(raw,&idx, 0,size);
	}}
}
void sysroff_swap_dus_in (struct IT_dus * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->efn  = getINT(raw,&idx, 2,size);
	ptr->ns  = getINT(raw,&idx, 2,size);
	{ int n; for (n = 0; n < ptr->ns; n++) {
if (!ptr->drb) ptr->drb = (INT*)xcalloc(ptr->ns, sizeof(ptr->drb[0]));
	ptr->drb[n] = getBITS(raw,&idx, 1,size);
if (!ptr->spare) ptr->spare = (INT*)xcalloc(ptr->ns, sizeof(ptr->spare[0]));
	ptr->spare[n] = getBITS(raw,&idx, 7,size);
if (!ptr->fname) ptr->fname = (CHARS*)xcalloc(ptr->ns, sizeof(ptr->fname[0]));
	ptr->fname[n] = getCHARS(raw,&idx, 0,size);
	if (ptr->drb[n]) {
if (!ptr->dan) ptr->dan = (INT*)xcalloc(ptr->ns, sizeof(ptr->dan[0]));
	ptr->dan[n] = getINT(raw,&idx, 2,size);
	}
	}}
	ptr->ndir  = getINT(raw,&idx, 2,size);
	{ int n; for (n = 0; n < ptr->ndir; n++) {
if (!ptr->dname) ptr->dname = (CHARS*)xcalloc(ptr->ndir, sizeof(ptr->dname[0]));
	ptr->dname[n] = getCHARS(raw,&idx, 0,size);
	}}
}
void sysroff_swap_dps_in (struct IT_dps * ptr)
{
	unsigned char raw[255];
	int idx = 0;
	int size;
	memset(raw,0,255);
	memset(ptr,0,sizeof(*ptr));
	size = fillup(raw);
	ptr->end  = getBITS(raw,&idx, 1,size);
	ptr->type  = getBITS(raw,&idx, 7,size);
	if (!ptr->end) {
	ptr->opt  = getINT(raw,&idx, 1,size);
	ptr->san  = getINT(raw,&idx, 2,size);
	ptr->address  = getINT(raw,&idx, -2,size);
	ptr->block_size  = getINT(raw,&idx, -2,size);
	ptr->nesting  = getINT(raw,&idx, 1,size);
	if (ptr->type == BLOCK_TYPE_PROCEDURE
	    || ptr->type == BLOCK_TYPE_FUNCTION) {
	ptr->retaddr  = getBITS(raw,&idx, 1,size);
	ptr->intrflag  = getBITS(raw,&idx, 1,size);
	ptr->stackflag  = getBITS(raw,&idx, 1,size);
	ptr->intrpagejmp  = getBITS(raw,&idx, 1,size);
	ptr->spare  = getBITS(raw,&idx, 4,size);
	}
	ptr->neg  = getINT(raw,&idx, 2,size);
	}
}
#endif
#ifdef SYSROFF_SWAP_OUT
void sysroff_swap_cs_out (FILE * ffile, struct IT_cs * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_cs_CODE;
	writeINT(ptr->size ,raw,&idx,1,ffile);
	writeINT(ptr->hd ,raw,&idx,1,ffile);
	writeINT(ptr->hs ,raw,&idx,1,ffile);
	writeINT(ptr->un ,raw,&idx,1,ffile);
	writeINT(ptr->us ,raw,&idx,1,ffile);
	writeINT(ptr->sc ,raw,&idx,1,ffile);
	writeINT(ptr->ss ,raw,&idx,1,ffile);
	writeINT(ptr->er ,raw,&idx,1,ffile);
	writeINT(ptr->ed ,raw,&idx,1,ffile);
	writeINT(ptr->sh ,raw,&idx,1,ffile);
	writeINT(ptr->ob ,raw,&idx,1,ffile);
	writeINT(ptr->rl ,raw,&idx,1,ffile);
	writeINT(ptr->du ,raw,&idx,1,ffile);
	writeINT(ptr->dps ,raw,&idx,1,ffile);
	writeINT(ptr->dsy ,raw,&idx,1,ffile);
	writeINT(ptr->dty ,raw,&idx,1,ffile);
	writeINT(ptr->dln ,raw,&idx,1,ffile);
	writeINT(ptr->dso ,raw,&idx,1,ffile);
	writeINT(ptr->dus ,raw,&idx,1,ffile);
	writeINT(ptr->dss ,raw,&idx,1,ffile);
	writeINT(ptr->dbt ,raw,&idx,1,ffile);
	writeINT(ptr->dpp ,raw,&idx,1,ffile);
	writeINT(ptr->dfp ,raw,&idx,1,ffile);
	writeINT(ptr->den ,raw,&idx,1,ffile);
	writeINT(ptr->dds ,raw,&idx,1,ffile);
	writeINT(ptr->dar ,raw,&idx,1,ffile);
	writeINT(ptr->dpt ,raw,&idx,1,ffile);
	writeINT(ptr->dul ,raw,&idx,1,ffile);
	writeINT(ptr->dse ,raw,&idx,1,ffile);
	writeINT(ptr->dot ,raw,&idx,1,ffile);
	checksum(ffile,raw, idx, IT_cs_CODE);
}
void sysroff_swap_hd_out (FILE * ffile, struct IT_hd * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_hd_CODE;
	writeBITS(ptr->mt ,raw,&idx,4);
	writeBITS(ptr->spare1 ,raw,&idx,4);
	writeCHARS(ptr->cd ,raw,&idx,12,ffile);
	writeINT(ptr->nu ,raw,&idx,2,ffile);
	writeINT(ptr->code ,raw,&idx,1,ffile);
	writeCHARS(ptr->ver ,raw,&idx,4,ffile);
	writeINT(ptr->au ,raw,&idx,1,ffile);
	writeBITS(ptr->si ,raw,&idx,1);
	writeBITS(ptr->afl ,raw,&idx,4);
	writeBITS(ptr->spare2 ,raw,&idx,3);
	writeINT(ptr->spcsz ,raw,&idx,1,ffile);
	writeINT(ptr->segsz ,raw,&idx,1,ffile);
	writeINT(ptr->segsh ,raw,&idx,1,ffile);
	writeINT(ptr->ep ,raw,&idx,1,ffile);
	if (ptr->ep) {
	if (ptr->mt != MTYPE_ABS_LM) {
	writeINT(ptr->uan ,raw,&idx,2,ffile);
	writeINT(ptr->sa ,raw,&idx,2,ffile);
	}
	if (segmented_p) {
	writeINT(ptr->sad ,raw,&idx,-1,ffile);
	}
	writeINT(ptr->address ,raw,&idx,-2,ffile);
	}
	writeCHARS(ptr->os ,raw,&idx,0,ffile);
	writeCHARS(ptr->sys ,raw,&idx,0,ffile);
	writeCHARS(ptr->mn ,raw,&idx,0,ffile);
	writeCHARS(ptr->cpu ,raw,&idx,0,ffile);
	checksum(ffile,raw, idx, IT_hd_CODE);
}
void sysroff_swap_hs_out (FILE * ffile, struct IT_hs * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_hs_CODE;
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_hs_CODE);
}
void sysroff_swap_un_out (FILE * ffile, struct IT_un * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_un_CODE;
	writeBITS(ptr->format ,raw,&idx,2);
	writeBITS(ptr->spare1 ,raw,&idx,6);
	writeINT(ptr->nsections ,raw,&idx,2,ffile);
	writeINT(ptr->nextrefs ,raw,&idx,2,ffile);
	writeINT(ptr->nextdefs ,raw,&idx,2,ffile);
	writeCHARS(ptr->name ,raw,&idx,0,ffile);
	writeCHARS(ptr->tool ,raw,&idx,0,ffile);
	writeCHARS(ptr->tcd ,raw,&idx,12,ffile);
	writeCHARS(ptr->linker ,raw,&idx,0,ffile);
	writeCHARS(ptr->lcd ,raw,&idx,12,ffile);
	checksum(ffile,raw, idx, IT_un_CODE);
}
void sysroff_swap_us_out (FILE * ffile, struct IT_us * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_us_CODE;
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_us_CODE);
}
void sysroff_swap_sc_out (FILE * ffile, struct IT_sc * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_sc_CODE;
	writeBITS(ptr->format ,raw,&idx,2);
	writeBITS(ptr->spare ,raw,&idx,6);
	writeINT(ptr->segadd ,raw,&idx,-1,ffile);
	writeINT(ptr->addr ,raw,&idx,-2,ffile);
	writeINT(ptr->length ,raw,&idx,-2,ffile);
	writeINT(ptr->align ,raw,&idx,-2,ffile);
	writeBITS(ptr->contents ,raw,&idx,4);
	writeBITS(ptr->concat ,raw,&idx,4);
	writeBITS(ptr->read ,raw,&idx,2);
	writeBITS(ptr->write ,raw,&idx,2);
	writeBITS(ptr->exec ,raw,&idx,2);
	writeBITS(ptr->init ,raw,&idx,2);
	writeBITS(ptr->mode ,raw,&idx,2);
	writeBITS(ptr->spare1 ,raw,&idx,6);
	writeCHARS(ptr->name ,raw,&idx,0,ffile);
	checksum(ffile,raw, idx, IT_sc_CODE);
}
void sysroff_swap_ss_out (FILE * ffile, struct IT_ss * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_ss_CODE;
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_ss_CODE);
}
void sysroff_swap_er_out (FILE * ffile, struct IT_er * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_er_CODE;
	writeBITS(ptr->type ,raw,&idx,2);
	writeBITS(ptr->spare ,raw,&idx,6);
	writeCHARS(ptr->name ,raw,&idx,0,ffile);
	checksum(ffile,raw, idx, IT_er_CODE);
}
void sysroff_swap_ed_out (FILE * ffile, struct IT_ed * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_ed_CODE;
	writeINT(ptr->section ,raw,&idx,2,ffile);
	writeBITS(ptr->type ,raw,&idx,3);
	writeBITS(ptr->spare ,raw,&idx,5);
	if (ptr->type==ED_TYPE_ENTRY || ptr->type==ED_TYPE_DATA) {
	writeINT(ptr->address ,raw,&idx,-2,ffile);
	}
	if (ptr->type==ED_TYPE_CONST) {
	writeINT(ptr->constant ,raw,&idx,-2,ffile);
	}
	writeCHARS(ptr->name ,raw,&idx,0,ffile);
	checksum(ffile,raw, idx, IT_ed_CODE);
}
void sysroff_swap_sh_out (FILE * ffile, struct IT_sh * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_sh_CODE;
	writeINT(ptr->unit ,raw,&idx,2,ffile);
	writeINT(ptr->section ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_sh_CODE);
}
void sysroff_swap_ob_out (FILE * ffile, struct IT_ob * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_ob_CODE;
	writeBITS(ptr->saf ,raw,&idx,1);
	writeBITS(ptr->cpf ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,6);
	if (ptr->saf) {
	writeINT(ptr->address ,raw,&idx,-2,ffile);
	}
	if (ptr->cpf) {
	writeINT(ptr->compreps ,raw,&idx,-2,ffile);
	}
	writeBARRAY(ptr->data ,raw,&idx,-4,ffile);
	checksum(ffile,raw, idx, IT_ob_CODE);
}
void sysroff_swap_rl_out (FILE * ffile, struct IT_rl * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_rl_CODE;
	writeBITS(ptr->boundary ,raw,&idx,4);
	writeBITS(ptr->apol ,raw,&idx,1);
	writeBITS(ptr->segment ,raw,&idx,1);
	writeBITS(ptr->sign ,raw,&idx,1);
	writeBITS(ptr->check ,raw,&idx,1);
	writeINT(ptr->addr ,raw,&idx,-2,ffile);
	writeINT(ptr->bitloc ,raw,&idx,1,ffile);
	writeINT(ptr->flen ,raw,&idx,1,ffile);
	writeINT(ptr->bcount ,raw,&idx,1,ffile);
	writeINT(ptr->op ,raw,&idx,1,ffile);
	if (ptr->op == OP_EXT_REF) {
	writeINT(ptr->symn ,raw,&idx,2,ffile);
	}
	if (ptr->op == OP_SEC_REF) {
	writeINT(ptr->secn ,raw,&idx,2,ffile);
	writeINT(ptr->copcode_is_3 ,raw,&idx,1,ffile);
	writeINT(ptr->alength_is_4 ,raw,&idx,1,ffile);
	writeINT(ptr->addend ,raw,&idx,4,ffile);
	writeINT(ptr->aopcode_is_0x20 ,raw,&idx,1,ffile);
	}
	if (ptr->op == OP_RELOC_ADDR) {
	writeINT(ptr->dunno ,raw,&idx,2,ffile);
	}
	writeINT(ptr->end ,raw,&idx,1,ffile);
	checksum(ffile,raw, idx, IT_rl_CODE);
}
void sysroff_swap_du_out (FILE * ffile, struct IT_du * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_du_CODE;
	writeBITS(ptr->format ,raw,&idx,2);
	writeBITS(ptr->optimized ,raw,&idx,1);
	writeBITS(ptr->stackfrmt ,raw,&idx,2);
	writeBITS(ptr->spare ,raw,&idx,3);
	writeINT(ptr->unit ,raw,&idx,2,ffile);
	writeINT(ptr->sections ,raw,&idx,2,ffile);
	{ int n; for (n = 0; n < ptr->sections; n++) {
	writeINT(ptr->san[n],raw,&idx,2,ffile);
	writeINT(ptr->address[n],raw,&idx,-2,ffile);
	writeINT(ptr->length[n],raw,&idx,-2,ffile);
	}}
	writeCHARS(ptr->tool ,raw,&idx,0,ffile);
	writeCHARS(ptr->date ,raw,&idx,12,ffile);
	checksum(ffile,raw, idx, IT_du_CODE);
}
void sysroff_swap_dsy_out (FILE * ffile, struct IT_dsy * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dsy_CODE;
	writeBITS(ptr->type ,raw,&idx,7);
	writeBITS(ptr->assign ,raw,&idx,1);
	writeINT(ptr->snumber ,raw,&idx,2,ffile);
	writeCHARS(ptr->sname ,raw,&idx,0,ffile);
	writeINT(ptr->nesting ,raw,&idx,2,ffile);
	if (ptr->assign) {
	writeINT(ptr->ainfo ,raw,&idx,1,ffile);
	writeINT(ptr->dlength ,raw,&idx,-2,ffile);
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
            || ptr->ainfo == AINFO_STATIC_INT
            || ptr->ainfo == AINFO_STATIC_COM) {
	writeINT(ptr->section ,raw,&idx,2,ffile);
	}
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
            || ptr->ainfo == AINFO_STATIC_INT
            || ptr->ainfo == AINFO_STATIC_COM
            || ptr->ainfo == AINFO_AUTO) {
	writeINT(ptr->address ,raw,&idx,-2,ffile);
	}
	if (ptr->ainfo == AINFO_REG) {
	writeCHARS(ptr->reg ,raw,&idx,0,ffile);
	}
	if (ptr->ainfo == AINFO_STATIC_EXT_DEF
	    || ptr->ainfo == AINFO_STATIC_EXT_REF) {
	writeCHARS(ptr->ename ,raw,&idx,0,ffile);
	}
	if (ptr->ainfo == AINFO_CONST) {
	writeCHARS(ptr->constant ,raw,&idx,0,ffile);
	}
	}
	if (ptr->type == STYPE_MEMBER) {
	writeBITS(ptr->bitunit ,raw,&idx,1);
	writeBITS(ptr->spare2 ,raw,&idx,7);
	writeINT(ptr->field_len ,raw,&idx,-2,ffile);
	writeINT(ptr->field_off ,raw,&idx,-2,ffile);
	if (ptr->bitunit) {
	writeINT(ptr->field_bitoff ,raw,&idx,-2,ffile);
	}
	}
	if (ptr->type== STYPE_ENUM) {
	writeINT(ptr->evallen ,raw,&idx,1,ffile);
	writeINT(ptr->evalue ,raw,&idx,4,ffile);
	}
	if (ptr->type == STYPE_CONST) {
	writeCHARS(ptr->cvalue ,raw,&idx,0,ffile);
	}
	if (ptr->type == STYPE_EQUATE) {
	writeINT(ptr->qvallen ,raw,&idx,1,ffile);
	writeINT(ptr->qvalue ,raw,&idx,4,ffile);
	writeINT(ptr->btype ,raw,&idx,1,ffile);
	writeINT(ptr->sizeinfo ,raw,&idx,-2,ffile);
	writeBITS(ptr->sign ,raw,&idx,2);
	writeBITS(ptr->flt_type ,raw,&idx,6);
	}
	writeINT(ptr->sfn ,raw,&idx,2,ffile);
	writeINT(ptr->sln ,raw,&idx,2,ffile);
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	if (ptr->type == STYPE_TAG) {
	writeINT(ptr->magic ,raw,&idx,1,ffile);
	}
	checksum(ffile,raw, idx, IT_dsy_CODE);
}
void sysroff_swap_dul_out (FILE * ffile, struct IT_dul * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dul_CODE;
	writeBITS(ptr->max_variable ,raw,&idx,1);
	writeBITS(ptr->maxspare ,raw,&idx,7);
	if (ptr->max_variable == 0) {
	writeINT(ptr->max ,raw,&idx,-2,ffile);
	writeCHARS(ptr->maxmode ,raw,&idx,0,ffile);
	}
	writeBITS(ptr->min_variable ,raw,&idx,1);
	writeBITS(ptr->minspare ,raw,&idx,7);
	if (ptr->min_variable == 0) {
	writeINT(ptr->min ,raw,&idx,-2,ffile);
	writeCHARS(ptr->minmode ,raw,&idx,0,ffile);
	}
	checksum(ffile,raw, idx, IT_dul_CODE);
}
void sysroff_swap_dty_out (FILE * ffile, struct IT_dty * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dty_CODE;
	writeBITS(ptr->end ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,7);
	if (!ptr->end) {
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	}
	checksum(ffile,raw, idx, IT_dty_CODE);
}
void sysroff_swap_dbt_out (FILE * ffile, struct IT_dbt * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dbt_CODE;
	writeINT(ptr->btype ,raw,&idx,1,ffile);
	writeINT(ptr->bitsize ,raw,&idx,-2,ffile);
	writeBITS(ptr->sign ,raw,&idx,2);
	writeBITS(ptr->fptype ,raw,&idx,6);
	if (ptr->btype==BTYPE_TAG || ptr->btype == BTYPE_TYPE) {
	writeINT(ptr->sid ,raw,&idx,2,ffile);
	}
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_dbt_CODE);
}
void sysroff_swap_dar_out (FILE * ffile, struct IT_dar * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dar_CODE;
	writeINT(ptr->length ,raw,&idx,-2,ffile);
	writeINT(ptr->dims ,raw,&idx,1,ffile);
	{ int n; for (n = 0; n < ptr->dims; n++) {
	writeBITS(ptr->variable[n],raw,&idx,1);
	writeBITS(ptr->subtype[n],raw,&idx,1);
	writeBITS(ptr->spare[n],raw,&idx,6);
	if (ptr->subtype[n] == SUB_TYPE) {
	writeINT(ptr->sid[n],raw,&idx,2,ffile);
	}
	if (ptr->subtype[n] == SUB_INTEGER) {
	writeBITS(ptr->max_variable[n],raw,&idx,1);
	writeBITS(ptr->maxspare[n],raw,&idx,7);
	writeINT(ptr->max[n],raw,&idx,-2,ffile);
	writeBITS(ptr->min_variable[n],raw,&idx,1);
	writeBITS(ptr->minspare[n],raw,&idx,7);
	writeINT(ptr->min[n],raw,&idx,-2,ffile);
	}
	}}
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_dar_CODE);
}
void sysroff_swap_dso_out (FILE * ffile, struct IT_dso * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dso_CODE;
	writeINT(ptr->sid ,raw,&idx,2,ffile);
	writeINT(ptr->spupdates ,raw,&idx,4,ffile);
	{ int n; for (n = 0; n < ptr->spupdates; n++) {
	writeINT(ptr->address[n],raw,&idx,-2,ffile);
	writeINT(ptr->offset[n],raw,&idx,-2,ffile);
	}}
	checksum(ffile,raw, idx, IT_dso_CODE);
}
void sysroff_swap_dln_out (FILE * ffile, struct IT_dln * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dln_CODE;
	writeINT(ptr->nln ,raw,&idx,2,ffile);
	{ int n; for (n = 0; n < ptr->nln; n++) {
	writeINT(ptr->sfn[n],raw,&idx,2,ffile);
	writeINT(ptr->sln[n],raw,&idx,2,ffile);
	writeINT(ptr->section[n],raw,&idx,2,ffile);
	writeINT(ptr->from_address[n],raw,&idx,-2,ffile);
	writeINT(ptr->to_address[n],raw,&idx,-2,ffile);
	writeINT(ptr->cc[n],raw,&idx,2,ffile);
	}}
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_dln_CODE);
}
void sysroff_swap_dpp_out (FILE * ffile, struct IT_dpp * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dpp_CODE;
	writeBITS(ptr->end ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,7);
	if (!ptr->end) {
	writeINT(ptr->params ,raw,&idx,1,ffile);
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	}
	checksum(ffile,raw, idx, IT_dpp_CODE);
}
void sysroff_swap_den_out (FILE * ffile, struct IT_den * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_den_CODE;
	writeBITS(ptr->end ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,7);
	if (!ptr->end) {
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	}
	checksum(ffile,raw, idx, IT_den_CODE);
}
void sysroff_swap_dfp_out (FILE * ffile, struct IT_dfp * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dfp_CODE;
	writeBITS(ptr->end ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,7);
	if (!ptr->end) {
	writeINT(ptr->nparams ,raw,&idx,1,ffile);
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	}
	checksum(ffile,raw, idx, IT_dfp_CODE);
}
void sysroff_swap_dds_out (FILE * ffile, struct IT_dds * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dds_CODE;
	writeBITS(ptr->end ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,7);
	if (!ptr->end) {
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	}
	checksum(ffile,raw, idx, IT_dds_CODE);
}
void sysroff_swap_dpt_out (FILE * ffile, struct IT_dpt * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dpt_CODE;
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	writeINT(ptr->dunno ,raw,&idx,1,ffile);
	checksum(ffile,raw, idx, IT_dpt_CODE);
}
void sysroff_swap_dse_out (FILE * ffile, struct IT_dse * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dse_CODE;
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	writeINT(ptr->dunno ,raw,&idx,1,ffile);
	checksum(ffile,raw, idx, IT_dse_CODE);
}
void sysroff_swap_dot_out (FILE * ffile, struct IT_dot * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dot_CODE;
	writeINT(ptr->unknown ,raw,&idx,1,ffile);
	checksum(ffile,raw, idx, IT_dot_CODE);
}
void sysroff_swap_dss_out (FILE * ffile, struct IT_dss * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dss_CODE;
	writeINT(ptr->type ,raw,&idx,1,ffile);
	writeBITS(ptr->internal ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,7);
	if (!ptr->internal) {
	writeCHARS(ptr->package ,raw,&idx,0,ffile);
	}
	if (ptr->internal) {
	writeINT(ptr->id ,raw,&idx,2,ffile);
	}
	writeINT(ptr->record ,raw,&idx,2,ffile);
	writeCHARS(ptr->rules ,raw,&idx,0,ffile);
	writeINT(ptr->nsymbols ,raw,&idx,2,ffile);
	writeINT(ptr->fixme ,raw,&idx,2,ffile);
	checksum(ffile,raw, idx, IT_dss_CODE);
}
void sysroff_swap_pss_out (FILE * ffile, struct IT_pss * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_pss_CODE;
	writeINT(ptr->efn ,raw,&idx,2,ffile);
	writeINT(ptr->ns ,raw,&idx,2,ffile);
	{ int n; for (n = 0; n < ptr->ns; n++) {
	writeBITS(ptr->drb[n],raw,&idx,1);
	writeBITS(ptr->spare[n],raw,&idx,7);
	writeCHARS(ptr->fname[n],raw,&idx,0,ffile);
	if (ptr->drb[n]) {
	writeINT(ptr->dan[n],raw,&idx,2,ffile);
	}
	}}
	writeINT(ptr->ndir ,raw,&idx,2,ffile);
	{ int n; for (n = 0; n < ptr->ndir; n++) {
	writeCHARS(ptr->dname[n],raw,&idx,0,ffile);
	}}
	checksum(ffile,raw, idx, IT_pss_CODE);
}
void sysroff_swap_dus_out (FILE * ffile, struct IT_dus * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dus_CODE;
	writeINT(ptr->efn ,raw,&idx,2,ffile);
	writeINT(ptr->ns ,raw,&idx,2,ffile);
	{ int n; for (n = 0; n < ptr->ns; n++) {
	writeBITS(ptr->drb[n],raw,&idx,1);
	writeBITS(ptr->spare[n],raw,&idx,7);
	writeCHARS(ptr->fname[n],raw,&idx,0,ffile);
	if (ptr->drb[n]) {
	writeINT(ptr->dan[n],raw,&idx,2,ffile);
	}
	}}
	writeINT(ptr->ndir ,raw,&idx,2,ffile);
	{ int n; for (n = 0; n < ptr->ndir; n++) {
	writeCHARS(ptr->dname[n],raw,&idx,0,ffile);
	}}
	checksum(ffile,raw, idx, IT_dus_CODE);
}
void sysroff_swap_dps_out (FILE * ffile, struct IT_dps * ptr)
{
	unsigned char raw[255];
	int idx = 16;
	memset (raw, 0, 255);
	code = IT_dps_CODE;
	writeBITS(ptr->end ,raw,&idx,1);
	writeBITS(ptr->type ,raw,&idx,7);
	if (!ptr->end) {
	writeINT(ptr->opt ,raw,&idx,1,ffile);
	writeINT(ptr->san ,raw,&idx,2,ffile);
	writeINT(ptr->address ,raw,&idx,-2,ffile);
	writeINT(ptr->block_size ,raw,&idx,-2,ffile);
	writeINT(ptr->nesting ,raw,&idx,1,ffile);
	if (ptr->type == BLOCK_TYPE_PROCEDURE
	    || ptr->type == BLOCK_TYPE_FUNCTION) {
	writeBITS(ptr->retaddr ,raw,&idx,1);
	writeBITS(ptr->intrflag ,raw,&idx,1);
	writeBITS(ptr->stackflag ,raw,&idx,1);
	writeBITS(ptr->intrpagejmp ,raw,&idx,1);
	writeBITS(ptr->spare ,raw,&idx,4);
	}
	writeINT(ptr->neg ,raw,&idx,2,ffile);
	}
	checksum(ffile,raw, idx, IT_dps_CODE);
}
#endif
