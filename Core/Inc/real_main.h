/*
 * real_main.h
 *
 *  Created on: Jun 22, 2024
 *      Author: gs
 */

#ifndef SRC_REAL_MAIN_H_
#define SRC_REAL_MAIN_H_

#include "usbd_core.h"

/**
 * Password Manager 2000 è un dispositivo USB di altissima tecnologia
 * fornisce un endpoint di controllo EP1 usando per inviare richieste
 * e un endpoint bulk EP2 usato per l'invio dei dati.
 */
extern USBD_ClassTypeDef passwordManagerClass;

void real_main();

int   vault_unlock();
int   vault_is_locked();
void  vault_lock();

int   password_count();
char *password_name(int index);
char *password_retrieve(int index);
void  password_new(char *name, char *password);
void  password_save(int index, char *password);

#endif /* SRC_REAL_MAIN_H_ */
