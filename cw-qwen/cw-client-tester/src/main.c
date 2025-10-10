#include "cw-client-tester.h"

// Main function
int main(int argc, char *argv[]) {
    printf("CAPWAP Client Tester Application\n");
    printf("Initializing...\n");
    
    // Initialize the application
    if (app_init(argc, argv) != 0) {
        fprintf(stderr, "Failed to initialize application\n");
        return 1;
    }
    
    // Run the application
    app_run();
    
    // Perform cleanup
    app_cleanup();
    
    printf("Application terminated\n");
    return 0;
}