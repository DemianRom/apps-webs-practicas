/*
 * Practica 5 - Servidor de chat no bloqueante
 * Materia: Aplicaciones y Comunicaciones en Red (6CM1)
 * ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)
 * Periodo: 26/2
 *
 * Integrantes:
 * - Romero Bautista Demian
 * - Ferreira Rodriguez Said
 *
 * Responsabilidad del archivo:
 * Implementa el modelo de cliente conectado, incluyendo estado de sesion, buffer y cola de salida.
 */

#include "cliente.hpp"
// La estructura Cliente solo tiene datos y un metodo inline (registrado()).
// Toda la logica que opera sobre clientes vive en chat_server.cpp. Este .cpp
// existe para darle una unidad de compilacion propia dentro del Makefile.
