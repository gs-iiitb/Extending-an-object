#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

struct object;
struct vtable;
typedef struct object *(*method_t)(void *dummy, void *vt, ...);

// element of zero size to make it easy to step back from oop to pick up
// its vtable
#define _VTABLE_REF struct vtable *_vt[0]

#define vtof(p)	((struct vtable *)((p)->_vt[-1]))
#define vt_set(p, t)	((p)->_vt[-1] = (struct vtable *)(t))

struct object_class {
	_VTABLE_REF;
	// slots begin here
};

struct object {
	_VTABLE_REF;
	// slots begin here
};

struct vtable {
	_VTABLE_REF;
	char *name;
	int size;
	int tally;
	struct symbol **keys;
	method_t       *values;
	struct vtable *parent;
};

struct symbol_class {
	_VTABLE_REF;
	// class variables go here 
};

struct symbol {
	_VTABLE_REF;
	// instance variables go here
	char *string;
};

struct ovm {
	struct vtable *atom_vt;
	struct vtable *proto_vt;
	struct vtable *object_vt;
	struct vtable *symbol_vt;

	struct object *Proto;
	struct object_class *Object;
	struct symbol_class *Symbol;
} Foo;

// allocate an object that uses a vtable
static struct object *vt_alloc(struct object *dummy, struct vtable *self, size_t bytes)
{
	struct vtable **m = (typeof(m))calloc(1, sizeof(m[0]) + bytes);
	struct object *oop;

	assert(m != 0);
	oop = (struct object *)(m+1);
	vt_set(oop, self);
	printf("alloc %p size %3ld = mem %p oop %p\n", self, bytes, m, oop);
	return oop;
}

#define object_alloc(vt, type)	((type *)vt_alloc(0, (vt), sizeof(type)))

// allocate a child vtable that delegates to a parent
static struct vtable *vt_delegate(struct object *dummy, struct vtable *parent, char *name)
{
	struct vtable *pvt = parent ? vtof(parent) : 0;
	struct vtable *child = object_alloc(pvt, struct vtable);

	vt_set(child, parent ? vtof(parent) : 0);
	child->name = strdup(name);
	child->size = 2;
	child->tally = 0;
	child->keys   = (typeof(child->keys))calloc(child->size,   sizeof(typeof(child->keys[0])));
	child->values = (typeof(child->values))calloc(child->size, sizeof(typeof(child->values[0])));
	child->parent = parent;
	return child;
}

static struct vtable *vt_print(struct object *dummy, struct vtable *self)
{
	assert(self);
	printf("vt %9s %p vt %p -> %p", self->name, self, vtof(self), self->parent);
	return self;
}

static struct symbol *symbol_alloc(char *string)
{
	size_t sbytes = strlen(string)+1;
	char *mem = (char *)vt_alloc(0, Foo.atom_vt, sizeof(struct symbol)+sbytes);
	struct symbol *sym = (struct symbol *)mem;

	vt_set(sym, Foo.symbol_vt);
	sym->string = mem + sizeof(struct symbol);
	strncpy(sym->string, string, sbytes);
	return sym;
}

static struct object *vt_selfie(struct object *dummy, struct vtable *vt)
{
	assert(vt);
	return (struct object *)vt;
}

static struct symbol *atom_add(struct vtable *vt, struct symbol *key, method_t method)
{
	assert(vt);
	if (vt->tally == vt->size) {
		vt->size *= 2;
		vt->keys   = (typeof(key)    *)realloc(vt->keys, vt->size*sizeof(key));
		vt->values = (typeof(method) *)realloc(vt->values, vt->size*sizeof(method));
	}
	vt->keys[vt->tally]     = key;
	vt->values[vt->tally++] = method;
	return key;
}

static struct symbol *atom(char *str)
{
	int i;
	struct symbol *k;
	struct vtable *vt = Foo.atom_vt;

	assert(vt);
	for (i = 0; i < vt->tally; i++) {
		if (strcmp(str, vt->keys[i]->string) == 0)
			return vt->keys[i];
	}
	k = symbol_alloc(str);
	return atom_add(vt, k, (method_t)vt_selfie);
}

static method_t vt_lookup(struct object *dummy, struct vtable *vt,
				struct symbol *key)
{
	int i;

	for (i = 0; i < vt->tally; i++) {
		if (key == vt->keys[i])
			return vt->values[i];
	}
	if (vt->parent && (vt->parent != vt))
		return vt_lookup(0, vt->parent, key);
	return (method_t)vt_selfie;
}

static struct symbol *vt_add_method(struct object *dummy, struct vtable *vt,
			struct symbol *key, method_t method)
{
	int i;

	for (i = 0; i < vt->tally; i++) {
		if (key == vt->keys[i])
			return key;
	}
	return atom_add(vt, key, method);
}

// built-in methods for symbols

//Symbol>>#new:string
static struct symbol *symbol_new(struct object *dummy, struct vtable *self,
				char *contents)
{
	struct symbol *s = object_alloc(Foo.symbol_vt, struct symbol);

	assert(s);
	s->string = strdup(contents);
	return s;
}

//Symbol>>#print
static struct symbol *symbol_print(struct object *dummy, struct symbol *self)
{
	assert(self);
	assert(self->string);
	printf("%s", self->string);
	return self;
}

void init_ovm(struct ovm *mach)
{

	mach->atom_vt   = vt_delegate(0, 0, "atom");

	// object_vt does not exist yet, so proto_vt has no parent
	mach->proto_vt  = vt_delegate(0, 0, "proto");
	vt_set(mach->proto_vt,  mach->proto_vt);

	// create object_vt
	mach->object_vt = vt_delegate(0, 0, "object");
	vt_set(mach->object_vt, mach->proto_vt);

	// object_vt can now become a parent of proto_vt
	mach->proto_vt->parent = mach->object_vt;

	// object_vt can now spawn other vtables
	mach->symbol_vt = vt_delegate(0, mach->object_vt, "symbol");

	mach->Proto   = object_alloc(mach->proto_vt,  struct object);
	mach->Object  = object_alloc(mach->object_vt, struct object_class);
	mach->Symbol  = object_alloc(mach->symbol_vt, struct symbol_class);

	vt_add_method(0, mach->proto_vt,  atom("lookup"),    (method_t)vt_lookup);
	vt_add_method(0, mach->proto_vt,  atom("addMethod"), (method_t)vt_add_method);
	vt_add_method(0, mach->object_vt, atom("print"),     (method_t)vt_print);
	vt_add_method(0, mach->symbol_vt, atom("new:"),      (method_t)symbol_new);
	vt_add_method(0, mach->symbol_vt, atom("print"),     (method_t)symbol_print);

	vt_print(0, mach->atom_vt);   printf("\n");
	vt_print(0, mach->proto_vt);  printf("\n");
	vt_print(0, mach->object_vt); printf("\n");
	vt_print(0, mach->symbol_vt); printf("\n");

	printf("Proto  %p vt %p -> %p\n", mach->Proto,  mach->proto_vt, mach->proto_vt->parent);
	printf("Object %p vt %p -> %p\n", mach->Object, mach->object_vt, mach->object_vt->parent);
	printf("Symbol %p vt %p -> %p\n", mach->Symbol, mach->symbol_vt, mach->symbol_vt->parent);

	assert(vtof(mach->proto_vt)  == mach->proto_vt);
	assert(vtof(mach->object_vt) == mach->proto_vt);
	assert(vtof(mach->symbol_vt) == mach->proto_vt);

	assert(mach->proto_vt->parent == mach->object_vt);
	assert(mach->symbol_vt->parent == mach->object_vt);
	// machine is ready
}

int main(int argc, char *argv[])
{
	printf("sizeof int     = %ld\n", sizeof(int));
	printf("sizeof pointer = %ld\n", sizeof(struct object *));
	printf("sizeof object  = %ld\n", sizeof(struct object));
	printf("sizeof vtable  = %ld\n", sizeof(struct vtable));
	printf("sizeof symbol  = %ld\n", sizeof(struct symbol));

	init_ovm(&Foo);

	char          *greet          = "Machine Foo is ready\n";
	struct symbol *s_new          = atom("new:");
	struct symbol *p;
	method_t       m;


	printf("Direct invocation of new: and print\n");
	p = symbol_new(0, Foo.symbol_vt, greet);
	symbol_print(0, p);

	// p := Symbol new: greet.
 	 
	printf("Testing keyword new: operator\n");
	m = vt_lookup(0, vtof(Foo.Symbol), s_new);
	assert(m == (method_t)symbol_new);
	p = (struct symbol *)(*m)(0, vtof(Foo.Symbol), greet);

	printf("symbol oop    %p\n",  p);
	printf("symbol vt     %p\n",  vtof(p));
	printf("symbol string %p\n",  p->string);

	// p print.
	printf("Testing unary print operator\n");
	m = vt_lookup(0, vtof(p), atom("print"));
	assert(m == (method_t)symbol_print);
	(*m)(0, p);
}
