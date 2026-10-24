#include "types.h"

#include <string.h>

const char *pifari_type_name(PifariType type) {
 switch (type) {
  case PT_INT: return "int";
  case PT_FLOAT: return "float";
  case PT_BOOL: return "bool";
  case PT_STR: return "str";
  case PT_LIST: return "list";
  case PT_F64ARRAY: return "array";
  case PT_RANGE: return "range";
  case PT_TASK: return "Task";
  case PT_SHARED: return "Shared";
  case PT_NULL: return "null";
  case PT_ANY: default: return "any";
 }
}

PifariType pifari_type_from_name(const char *name) {
 if (!name) return PT_ANY;
 if (!strcmp(name, "int") || !strcmp(name, "long") || !strcmp(name, "short") ||
     !strcmp(name, "char") || !strcmp(name, "signed") || !strcmp(name, "unsigned")) return PT_INT;
 if (!strcmp(name, "float") || !strcmp(name, "double")) return PT_FLOAT;
 if (!strcmp(name, "bool")) return PT_BOOL;
 if (!strcmp(name, "str") || !strcmp(name, "txt")) return PT_STR;
 if (!strcmp(name, "list")) return PT_LIST;
 if (!strcmp(name, "array")) return PT_F64ARRAY;
 if (!strcmp(name, "Task")) return PT_TASK;
 if (!strcmp(name, "Agent")) return PT_ANY; /* Agent é chamada nativa, não tipo de valor */
 return PT_ANY;
}

int pifari_type_is_known(const char *name) {
 if (!name || !*name) return 0;
 if (!strcmp(name, "void")) return 0; /* void não é tipo de variável */
 return pifari_type_from_name(name) != PT_ANY;
}

int pifari_type_assignable(PifariType annotated, PifariType actual) {
 if (annotated == PT_ANY) return 1;
 if (annotated == actual) return 1;
 if (annotated == PT_FLOAT && actual == PT_INT) return 2;
 return 0;
}
