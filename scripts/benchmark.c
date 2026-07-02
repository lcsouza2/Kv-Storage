#include "user_interface.h"
#include "logging.h"
#include "utils.h"
#include "settings.h"
#include "compactor.h"
#include <stdio.h>
#include <stdlib.h>

void run_insertion_benchmark(Database *db) {
    info("Starting insertion benchmark...");

    FILE *f = fopen("metrics_insert.csv", "w");
    if (!f) {
        error("Erro ao criar metrics_insert.csv\n");
        return;
    }
    fprintf(f, "Carga_Acumulada,Tempo_Lote_Segundos,Insercoes_Por_Segundo\n");

    int batches[] = {10000, 40000, 50000, 100000};
    int batches_count = sizeof(batches) / sizeof(batches[0]);
    int inserted_keys = 0;

    for (int w = 0; w < batches_count; w++) {
        int target = batches[w];
        info("Injecting %d keys...\n", target);

        timer_type start_time, end_time;

        GET_TIME(start_time);
        for (int i = 0; i < target; i++) {
            char key[32], value[64];
            sprintf(key, "key_%08d", inserted_keys * 2);
            sprintf(value, "payload_data_%d", inserted_keys);
            db_insert(db, key, value);
            inserted_keys++;
        }

        GET_TIME(end_time);
        double time_taken = DIFF_TIME(start_time, end_time);
        double ops_per_sec = target / time_taken;

        fprintf(f, "%d,%.4f,%.2f\n", inserted_keys, time_taken, ops_per_sec);
        info("Batch completed: %.4f s (%.2f ops/s)\n", time_taken, ops_per_sec);
    }

    fclose(f);
}

void run_bloom_filter_benchmark(Database *db) {
    FILE *f = fopen("metrics_bloom.csv", "w");
    if (!f) {
        error("Error creating metrics_bloom.csv\n");
        return;
    }
    fprintf(f, "Tipo_Leitura,Tempo_Total_Segundos,Consultas_Por_Segundo\n");

    int num_queries = 20000;

    timer_type start_time, end_time;

    info("Testing %d positive reads (goes to disk)...\n", num_queries);
    GET_TIME(start_time);
    for (int i = 0; i < num_queries; i++) {
        char key[32];
        sprintf(key, "key_%08d", i * 2);
        char *val = db_select(db, key);
        if (val) free(val);
    }
    GET_TIME(end_time);
    double time_pos = DIFF_TIME(start_time, end_time);
    fprintf(f, "Positive (Goes to Disk),%.4f,%.2f\n", time_pos, num_queries / time_pos);
    info("Positive reads completed: %.4f s\n", time_pos);

    info("Testing %d negative reads (bloom filter activated)...\n", num_queries);
    GET_TIME(start_time);
    for (int i = 0; i < num_queries; i++) {
        char key[32];
        sprintf(key, "key_%08d", (i * 2) + 1);
        char *val = db_select(db, key);
        if (val) free(val);
    }
    GET_TIME(end_time);
    double time_neg = DIFF_TIME(start_time, end_time);
    fprintf(f, "Negative (Blocked by Bloom),%.4f,%.2f\n", time_neg, num_queries / time_neg);
    info("Negative reads completed: %.4f s\n", time_neg);

    fclose(f);
}

int main() {
    info("Preparing performance test environment...\n");

    create_data_storage_directory();

    Database *db = malloc(sizeof(Database));
    db->is_running = 1;
    init_background_compactor();

    run_insertion_benchmark(db);

    info("\nWaiting for asynchronous compaction to consolidate SSTables...\n");
    shutdown_background_compactor();

    init_background_compactor();
    run_bloom_filter_benchmark(db);
    shutdown_background_compactor();

    db->is_running = 0;
    free(db->memtable);
    free(db);
    info("\nBenchmarks completed successfully! CSV files generated in the root directory.\n");
    return 0;
}