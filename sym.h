#ifndef SYM_H
#define SYM_H

enum sym_type
{
	SYM_INT,
	SYN_FLOAT
};

struct sym_info
{
	char* name;
	enum sym_type type;	
};

struct sym_lookup_entry
{
	int hash;
	struct sym_info* sym;
};

struct sym_lookup_table
{
	int count;	
	struct sym_lookup_entry* symbols;
};


#endif
