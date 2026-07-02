#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_interface.h"
#include "logging.h"


static char *prompt_input(const char *prompt) {
    if (prompt) {
        printf("%s", prompt);
    }
    return read_dynamic_input();
}

int main() {
    Database *db = malloc(sizeof(Database));

    if (!db) {
        error("Failed to allocate database instance.");
        return EXIT_FAILURE;
    }

    if (boot_sync(db) != 0) {
        error("Failed to boot and synchronize the database.");
        clear_memtable(db->memtable);
        free(db->memtable);
        free(db);
        return EXIT_FAILURE;
    }

    while (1) {
        display_separator();
        display_menu();
        char *choice_buffer = prompt_input(NULL);
        if (!choice_buffer) {
            break;
        }

        int choice = atoi(choice_buffer);
        free(choice_buffer);

        if (choice == 1) {
            char *key = prompt_input("Key: ");
            char *value = prompt_input("Value: ");
            if (key && value) {
                db_insert(db, key, value);
            }
            free(key);
            free(value);
        } else if (choice == 2) {
            char *key = prompt_input("Key: ");
            if (!key) {
                continue;
            }

            char *value = db_select(db, key);
            if (value) {
                printf("Value: %s\n", value);
            } else {
                printf("Key not found.\n");
            }
            free(key);
        } else if (choice == 3) {
            char *key = prompt_input("Key: ");
            char *value = prompt_input("New value: ");
            if (key && value) {
                db_update(db, key, value);
            }
            free(key);
            free(value);
        } else if (choice == 4) {
            char *key = prompt_input("Key: ");
            if (key) {
                db_delete(db, key);
            }
            free(key);
        } else if (choice == 5) {
            display_all_keys(db);
        } else if (choice == 6) {
            clear();
        } else if (choice == 7) {
            break;
        } else {
            printf("Invalid option. Please try again.\n");
        }
    }

    clear_memtable(db->memtable);
    free(db->memtable);
    free(db);
    return EXIT_SUCCESS;

}