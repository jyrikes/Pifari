#ifndef PIFARI_DIAGNOSTICS_H
#define PIFARI_DIAGNOSTICS_H

/* Shared by lexer and parser. Recovery never authorizes execution. */
#define PIFARI_MAX_ERRORS 50
extern int pifari_frontend_errors;
void pifari_diagnostic(const char *phase, int line, int column, const char *message);

#endif
