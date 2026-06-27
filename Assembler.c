#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char name[20];
    int address;
} Symbol;

int main() {
    FILE* in = fopen("input.asm", "r");
    FILE* out = fopen("output.hex", "w");
    if (!in) { printf("파일 오류: input.asm을 찾을 수 없습니다.\n"); return 1; }

    char line[100];
    Symbol table[100];
    int symbol_count = 0;
    int location_counter = 0;

    // --- PASS 1: 심볼 테이블 생성 ---
    while (fgets(line, sizeof(line), in)) {
        char word1[20], word2[20], word3[20];
        int res = sscanf(line, "%s %s %s", word1, word2, word3);
        if (res <= 0) continue;

        if (word1[strlen(word1) - 1] == ',') {
            char label[20];
            strcpy(label, word1);
            label[strlen(label) - 1] = '\0';
            strcpy(table[symbol_count].name, label);
            table[symbol_count].address = location_counter;
            symbol_count++;
        }

        if (strcmp(word1, "ORG") == 0) {
            location_counter = (int)strtol(word2, NULL, 16);
            continue;
        }
        if (strcmp(word1, "END") == 0) break;
        location_counter++;
    }

    // --- PASS 2: 실제 번역 ---
    rewind(in);
    location_counter = 0; // 초기화 후 ORG를 만나면 다시 설정됨

    while (fgets(line, sizeof(line), in)) {
        char c1[20], c2[20], c3[20], c4[20]; // c4 추가
        // 단어를 4개까지 스캔하도록 수정
        int res = sscanf(line, "%s %s %s %s", c1, c2, c3, c4);
        if (res <= 0 || strcmp(c1, "END") == 0) break;

        char* current_cmd, * current_op = NULL;
        int indirect = 0; // 간접 주소 비트 초기화

        // 라벨이 있는 경우 (예: LOOP, LDA PTR I)
        if (c1[strlen(c1) - 1] == ',') {
            current_cmd = c2;
            if (res >= 3) current_op = c3;
            // 4번째 단어가 "I"인지 확인
            if (res == 4 && strcmp(c4, "I") == 0) indirect = 0x8000;
        }
        // 라벨이 없는 경우 (예: LDA PTR I)
        else {
            current_cmd = c1;
            if (res >= 2) current_op = c2;
            // 3번째 단어가 "I"인지 확인
            if (res == 3 && strcmp(c3, "I") == 0) indirect = 0x8000;
        }

        if (strcmp(current_cmd, "ORG") == 0) {
            location_counter = (int)strtol(current_op, NULL, 16);
            continue;
        }

        int machine_code = 0;
        int is_pseudo = 0;

        // 수정된 indirect 변수 적용
        if (strcmp(current_cmd, "ADD") == 0) machine_code = 0x1000 | indirect;
        else if (strcmp(current_cmd, "LDA") == 0) machine_code = 0x2000 | indirect;
        else if (strcmp(current_cmd, "STA") == 0) machine_code = 0x3000 | indirect;
        else if (strcmp(current_cmd, "BUN") == 0) machine_code = 0x4000 | indirect;
        else if (strcmp(current_cmd, "ISZ") == 0) machine_code = 0x6000 | indirect;
        else if (strcmp(current_cmd, "CLA") == 0) { fprintf(out, "7800\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "CLE") == 0) { fprintf(out, "7400\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "CMA") == 0) { fprintf(out, "7200\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "CME") == 0) { fprintf(out, "7100\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "CIR") == 0) { fprintf(out, "7080\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "CIL") == 0) { fprintf(out, "7040\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "INC") == 0) { fprintf(out, "7020\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "SPA") == 0) { fprintf(out, "7010\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "SNA") == 0) { fprintf(out, "7008\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "SZA") == 0) { fprintf(out, "7004\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "SZE") == 0) { fprintf(out, "7002\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "HLT") == 0) { fprintf(out, "7001\n"); is_pseudo = 1; }
        else if (strcmp(current_cmd, "HEX") == 0) { fprintf(out, "%04X\n", (int)strtol(current_op, NULL, 16)); is_pseudo = 1; }
        else if (strcmp(current_cmd, "DEC") == 0) { fprintf(out, "%04X\n", (int)atoi(current_op) & 0xFFFF); is_pseudo = 1; }

        if (!is_pseudo) {
            int found_addr = -1;
            for (int i = 0; i < symbol_count; i++) {
                if (current_op && strcmp(table[i].name, current_op) == 0) {
                    found_addr = table[i].address;
                    break;
                }
            }
            if (found_addr == -1 && current_op) found_addr = (int)strtol(current_op, NULL, 16);
            fprintf(out, "%04X\n", machine_code + (found_addr & 0x0FFF));
        }
        location_counter++;
    }
    fclose(in); fclose(out);
    printf("번역 완료! output.hex가 생성되었습니다.\n");
    return 0;
}