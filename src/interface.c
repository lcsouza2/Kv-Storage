#include <stdio.h>
#include "memtable.h"

void clear() {
    printf("\033[H\033[J");
}

void print_main_menu() {
    clear();
    printf("=== Main Menu ===\n");
    printf("1. Add Key-Value Pair\n");
    printf("2. Get Value by Key\n");
    printf("3. Update Value by Key\n");
    printf("4. Delete Key-Value Pair\n");
    printf("5. Display All Key-Value Pairs\n");
    printf("6. Exit\n");
    printf("Choose an option: ");
}


int main() {
    while (1) {
        print_main_menu();
        int choice;
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                // Add Key-Value Pair
                break;
            case 2:
                // Get Value by Key
                break;
            case 3:
                // Update Value by Key
                break;
            case 4:
                // Delete Key-Value Pair
                break;
            case 5:
                // Display All Key-Value Pairs
                break;
            case 6:
                printf("Exiting...\n");
                return 0;
            default:
                printf("Invalid option. Please try again.\n");
        }
    }
    return 0;
}