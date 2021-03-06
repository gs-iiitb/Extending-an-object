#include "ovm1.c"
#include "string.h"
// Begin String definitions
struct mystring { _VTABLE_REF; int size; int length; char *chars; }; 
typedef struct mystring *HString;

static struct vtable *String_vt;
static struct object *String;

// String>>#new:
static struct object *String_newp(struct closure *cls, struct object *self, char *chars)
{
	HString clone = (HString)send(vtof(self), s_vtallocate, sizeof(struct mystring));

	clone->length = strlen(chars);
	clone->chars  = strdup(chars);
	((struct object *)clone)->size+=clone->length; //storing the size+no.of characters
	return (struct object *)clone;
}

// String>>#length
static struct object *String_length(struct closure *cls, HString self) { return i2oop(self->length); }

// String>>#print
static struct object * String_print(struct closure * cls, HString self)
{
	int i;

	for (i = 0; i < self->length; i++)
		putchar(self->chars[i]);
	return (struct object * )self;
}
// String>>#append
static struct object *String_append(struct closure * cls, struct object * self, HString str1, HString str2)
{

	char *cop=malloc((str1->length+str2->length+1)*sizeof(char));
	strcpy(cop,str1->chars);
	strcat(cop,str2->chars);
	printf("%s",cop);
	struct object *clone=send(String,s_newp,cop);
	free(cop);
	return (struct object *)clone;
}static struct symbol *s_append;

// ------------------------ Begin Array definitions
struct array { _VTABLE_REF; int length; struct object **contents; };
typedef struct array *HArray;

static struct vtable *Array_vt;
static struct object *Array;

//Array>>#new:
static struct object *Array_newp(struct closure *cls, struct object *self, int length)
{
	HArray clone = (HArray)send(vtof(self), s_vtallocate, sizeof(struct array));

	clone->length   = length;
	clone->contents = (struct object **)calloc(clone->length, sizeof(struct object *));
	assert(clone->contents);
	((struct object *)clone)->size+=((clone->length)*sizeof(*(clone->contents)));
	return (struct object *)clone;
}

//Array>>#length
static struct object *Array_length(struct closure * cls, HArray self) { return i2oop(self->length); }

//Array>>#at:
static struct object *Array_at(struct closure *cls, HArray self, int ix)
{
	// index starts at 1
	if (0 < ix && ix <= self->length)
		return self->contents[ix-1];
	return 0;
}

//Array>>#at:put:
static struct object *Array_atput(struct closure *cls, HArray self, int ix, struct object *rval)
{
	// index starts at 1
	if (0 < ix && ix <= self->length)
		return self->contents[ix-1] = rval;
	return rval;
}

static struct symbol *s_at;
static struct symbol *s_atput;

int main(int argc, char *argv[])
{
	init_ovm();
	int i;

	s_at    = (typeof(s_at))   send(Symbol, s_newp, "at:");
	s_atput = (typeof(s_atput))send(Symbol, s_newp, "at:put:");
	s_append = (typeof(s_append))send(Symbol, s_newp, "append:");


	printf("Testing String\n");
	String_vt = (typeof(String_vt))send(Object_vt, s_vtdelegate, "String");
	String    = (typeof(String))send((struct object *)String_vt, s_vtallocate, 0);

	assert(vtof(String) == String_vt);

	send(String_vt, s_vtadd_method, s_newp,   (method_t)String_newp);
	send(String_vt, s_vtadd_method, s_length, (method_t)String_length);
	send(String_vt, s_vtadd_method, s_print,  (method_t)String_print);
	send(String_vt, s_vtadd_method, s_append, (method_t)String_append);

	struct object *greet = send(String, s_newp, "Object Machine v1.0\n");
	struct object *h     = send(String, s_newp, "hello");
	struct object *sp    = send(String, s_newp, " ");
	struct object *w     = send(String, s_newp, "world");
	struct object *nl    = send(String, s_newp, "\n");

	send(greet, s_print);
	printf("hello length %d\n", oop2i(send(h, s_length)));

	send(h,  s_print);
	send(sp, s_print);
	send(w,  s_print);
	send(nl, s_print);

	printf("Testing Array\n");
	Array_vt  = (typeof(Array_vt)) send(Object_vt, s_vtdelegate, "Array");
	Array     = (typeof(Array))    send((struct object *)Array_vt,  s_vtallocate, 0);

	assert(vtof(Array) == Array_vt);

	send(Array_vt,  s_vtadd_method, s_newp,   (method_t)Array_newp);
	send(Array_vt,  s_vtadd_method, s_length, (method_t)Array_length);
	send(Array_vt,  s_vtadd_method, s_at,     (method_t)Array_at);
	send(Array_vt,  s_vtadd_method, s_atput,  (method_t)Array_atput);

	struct object *line = send(Array, s_newp, 4);

	printf("array elements %d\n", oop2i(send(line, s_length)));
	send(line, s_atput, 1, h);
	send(line, s_atput, 2, sp);
	send(line, s_atput, 3, w); 
	send(line, s_atput, 4, nl);
	for (i = 1; i <= 4; i++)
		send(send(line, s_at, i), s_print);
	printf("\n#######################\n");
	struct object *jo=send(String, s_append, h, w);
	//struct object *str3=send(String, s_append, h,sp,);
	//send(String,s_append,h,w);
	send(String, s_append,h,sp);
	send(String,s_append,w,nl);
	//int space=oop2i(send(send(String,s_append,h,sp), s_sizeInMemory));
	printf("\n#######################\n");
	int p;
	p=oop2i(send(Symbol, s_sizeInMemory));
	printf("Size in memory of Symbol is %d\n",p);
	p=oop2i(send(Object, s_sizeInMemory));
	printf("Size in memory of Object is %d\n",p);
	p=oop2i(send(Proto, s_sizeInMemory));
	printf("Size in memory of Proto is %d\n",p);
	p=oop2i(send(String, s_sizeInMemory));
	printf("Size in memory of String is %d\n",p);
	p=oop2i(send(Array, s_sizeInMemory));
	printf("Size in memory of Array is %d\n",p);
	p=oop2i(send(w, s_sizeInMemory));
	printf("Size in memory of w is %d\n",p);
	p=oop2i(send(nl, s_sizeInMemory));
	printf("Size in memory of String nl is %d\n",p);
	p=oop2i(send(s_at, s_sizeInMemory));
	printf("Size in memory of Symbol s_at is %d\n",p);
	//long space=oop2i(send(Closure_vt, s_sizeInMemory));
	//printf("Size of closure_vt %lu\n",space);

	int c1=oop2i(send(h, s_length));
	int c2=oop2i(send(w, s_length));
	int c3=oop2i(send(jo, s_length));
	if(c1+c2==c3)
	printf("(h length + w length) = (h append: w) length. True\n");
	else
	printf("(h length + w length) = (h append: w) length. False\n");

	int c4=oop2i(send(h, s_sizeInMemory));
	int c5=oop2i(send(w, s_sizeInMemory));
	int c6=oop2i(send(jo, s_sizeInMemory));
	printf("size of h+w=%d and size of appended string=%d \n",(c4+c5),c6);
	if(c4+c5==c6)
	printf("(h sizeInMemory + w sizeInMemory) = (h append: w) sizeInMemory. True\n");
	else
	printf("(h sizeInMemory + w sizeInMemory) = (h append: w) sizeInMemory. False\n");
	return 0;
}
