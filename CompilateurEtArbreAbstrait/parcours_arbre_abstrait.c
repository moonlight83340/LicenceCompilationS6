#include <stdio.h>
#include "syntabs.h"
#include "util.h"
#include "tabsymboles.h"
#include "parcours_arbre_abstrait.h"

extern int portee;
extern int adresseLocaleCourante;
extern int adresseArgumentCourant;
int paramcpt;

/*-------------------------------------------------------------------------*/

void parcours_n_prog(n_prog *n){
	portee = P_VARIABLE_GLOBALE;
	adresseLocaleCourante = 0;
	adresseArgumentCourant = 0;
	if(showIntel){
		printf("\n%%include \"io.asm\"\nsection .bss\nsinput: resb 255\n");	
	}
	parcours_l_dec(n->variables);
	if(showIntel){
		printf("global _start\n_start:\ncall main\nmov eax, 1 ; 1 est le code de SYS_EXIT\nint 0x80 ; exit\nmain:\n");
	}
	parcours_l_dec(n->fonctions);
	if(showIntel){
		printf("ret\n");
	}
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

void parcours_l_instr(n_l_instr *n){
  if(n){ 
	  parcours_instr(n->tete);
	  parcours_l_instr(n->queue); 
  }
}

/*-------------------------------------------------------------------------*/

void parcours_instr(n_instr *n){
  if(n){
    if(n->type == blocInst) parcours_l_instr(n->u.liste);
    else if(n->type == affecteInst) parcours_instr_affect(n);
    else if(n->type == siInst) parcours_instr_si(n);
    else if(n->type == tantqueInst) parcours_instr_tantque(n);
    else if(n->type == appelInst) parcours_instr_appel(n);
    else if(n->type == retourInst) parcours_instr_retour(n);
    else if(n->type == ecrireInst) parcours_instr_ecrire(n);
  }
}

/*-------------------------------------------------------------------------*/

void parcours_instr_si(n_instr *n){  
	parcours_exp(n->u.si_.test);
	parcours_instr(n->u.si_.alors);
	if(n->u.si_.sinon){
		parcours_instr(n->u.si_.sinon);
	}
}

/*-------------------------------------------------------------------------*/

void parcours_instr_tantque(n_instr *n){
  parcours_exp(n->u.tantque_.test);
  parcours_instr(n->u.tantque_.faire);
}

/*-------------------------------------------------------------------------*/

void parcours_instr_affect(n_instr *n){
	if(showIntel){
		
	}
	
	parcours_var(n->u.affecte_.var);
	parcours_exp(n->u.affecte_.exp); 
	
	if (showIntel){

	}
}

/*-------------------------------------------------------------------------*/

void parcours_instr_appel(n_instr *n){
  parcours_appel(n->u.appel); 
}
/*-------------------------------------------------------------------------*/

void parcours_appel(n_appel *n){
	int fonc_id = rechercheExecutable(n->fonction);
	if (fonc_id >= 0){
		parcours_l_exp(n->args);  				
	}else{
		printf("Nom de fonction inconnue : %s\n", n->fonction);
	}
}

/*-------------------------------------------------------------------------*/

void parcours_instr_retour(n_instr *n){
	parcours_exp(n->u.retour_.expression);
}

/*-------------------------------------------------------------------------*/

void parcours_instr_ecrire(n_instr *n){
	parcours_exp(n->u.ecrire_.expression);
	if (showIntel){
      printf("\tpop eax\n\tcall iprintLF\n");
	}
}

/*-------------------------------------------------------------------------*/

void parcours_l_exp(n_l_exp *n){
	if(n){
		parcours_exp(n->tete);
		parcours_l_exp(n->queue);
	} 
}

/*-------------------------------------------------------------------------*/

void parcours_exp(n_exp *n){
	if(n->type == varExp) parcours_varExp(n);
	else if(n->type == opExp) parcours_opExp(n);
	else if(n->type == intExp) parcours_intExp(n);
	else if(n->type == appelExp) parcours_appelExp(n);
	else if(n->type == lireExp) parcours_lireExp(n);
}

/*-------------------------------------------------------------------------*/

void parcours_varExp(n_exp *n){
	parcours_var(n->u.var);  
}

/*-------------------------------------------------------------------------*/
void parcours_opExp(n_exp *n){
	if( n->u.opExp_.op1 != NULL ) {
		parcours_exp(n->u.opExp_.op1);
	}
	if( n->u.opExp_.op2 != NULL ) {
		parcours_exp(n->u.opExp_.op2);
	} 
}

/*-------------------------------------------------------------------------*/

void parcours_intExp(n_exp *n){

}

/*-------------------------------------------------------------------------*/
void parcours_lireExp(n_exp *n){

}

/*-------------------------------------------------------------------------*/

void parcours_appelExp(n_exp *n){
	parcours_appel(n->u.appel);
}

/*-------------------------------------------------------------------------*/

void parcours_l_dec(n_l_dec *n){
	if( n ){   
		parcours_dec(n->tete);
		parcours_l_dec(n->queue);   
	}
}

/*-------------------------------------------------------------------------*/

void parcours_dec(n_dec *n){
	if(n){
		paramcpt++;
		if(n->type == foncDec) {
			parcours_foncDec(n);
		}
		else if(n->type == varDec) {
			parcours_varDec(n);
		}
		else if(n->type == tabDec) { 
			parcours_tabDec(n);
		}
	}
}

/*-------------------------------------------------------------------------*/

void parcours_foncDec(n_dec *n){
	int fonc_id = rechercheDeclarative(n->nom);
	if (fonc_id >= 0){
		printf("Nom de fonction deja utilise : %s\n", n->nom);
	}else{
		// Ajout de la fonction
		paramcpt=0;
		int id=ajouteIdentificateur(n->nom, portee, T_FONCTION, adresseLocaleCourante, 0);
		entreeFonction();
		parcours_l_dec(n->u.foncDec_.param);
		tabsymboles.tab[id].complement = paramcpt;
		portee = P_VARIABLE_LOCALE;
		parcours_l_dec(n->u.foncDec_.variables);
		parcours_instr(n->u.foncDec_.corps);
		sortieFonction(trace_tabsymb);
	}

}

/*-------------------------------------------------------------------------*/

void parcours_varDec(n_dec *n){
	int var_id = rechercheDeclarative(n->nom); // On cherche si "nom" existe deja
	// Verifier qu'elles n'ont pas la meme portee
	if((var_id >= 0) && (tabsymboles.tab[var_id].portee != portee) || (var_id == -1)){
		ajouteIdentificateur(n->nom, portee, T_ENTIER, adresseLocaleCourante, 1);
		adresseLocaleCourante += 4;
		if(showIntel){
			if(portee == P_VARIABLE_GLOBALE){
				printf("\t%s resw 1\n", n->nom);
			}
		}
	}else{
		printf("Variable déjà declarée : %s\n", n->nom);
	}
}

/*-------------------------------------------------------------------------*/

void parcours_tabDec(n_dec *n){
	int var_id = rechercheDeclarative(n->nom);
	// Verifier qu'elles n'ont pas la meme portee
	if((var_id >= 0) && (tabsymboles.tab[var_id].portee != portee) || (var_id == -1)){
		ajouteIdentificateur(n->nom, portee, T_TABLEAU_ENTIER, adresseLocaleCourante, n->u.tabDec_.taille);
		adresseLocaleCourante += 4*(n->u.tabDec_.taille);
		if(showIntel){
			if(portee == P_VARIABLE_GLOBALE){
				printf("\t%s resd %d\n", n->nom,n->u.tabDec_.taille);
			}
		}
	}else{
		printf("Variable déjà declarée : %s\n", n->nom);
	}
}

/*-------------------------------------------------------------------------*/

void parcours_var(n_var *n){
	//printf("n->nom : %s\n n->type : %d\n simple : %d\n",n->nom,n->type,indicee);
	if(n->type == simple) {
		parcours_var_simple(n);
	}else if(n->type == indicee) {
		parcours_var_indicee(n);
	}
}

/*-------------------------------------------------------------------------*/
void parcours_var_simple(n_var *n){
	int var_id = rechercheExecutable(n->nom); // On cherche si "nom" existe
	if (var_id >= 0){ // Si on trouve var_id
		if(tabsymboles.tab[var_id].type != T_ENTIER){
			printf("Mauvais type (Entier attendu) : %s\n ", n->nom);
		}
	}else{
		printf("Variable non declaree : %s\n", n->nom);
	}
}

/*-------------------------------------------------------------------------*/
void parcours_var_indicee(n_var *n){
	int var_id = rechercheExecutable(n->nom); // On cherche si "nom" existe
	if (var_id >= 0){ // Si on trouve var_id
		if(tabsymboles.tab[var_id].type != T_TABLEAU_ENTIER){
			printf("Mauvais type (Tableau attendu) : %s\n", n->nom);
		}else{
			parcours_exp( n->u.indicee_.indice );
		}
	}else{
		printf("Variable non declaree : %s\n", n->nom);
	}
}
/*-------------------------------------------------------------------------*/
