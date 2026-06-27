#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int M[4096] = { 0 };
    int PC = 0, AR = 0, IR = 0, AC = 0, DR = 0, SC = 0, I = 0, Opcode = 0, E = 0;

    FILE* fp = fopen("output.hex", "r");
    if (fp == NULL) {
        printf("오류: output.hex 파일을 찾을 수 없습니다. 어셈블러를 먼저 실행하세요.\n");
        return 1;
    }

    int addr = 0;
    while (fscanf(fp, "%x", &M[addr]) != EOF) {
        addr++;
    }
    fclose(fp);
    printf("시스템: %d개의 단어를 메모리에 로드했습니다.\n\n", addr);
    // ----------------------------------------------

    while (1) {
        if (SC == 0) {
            AR = PC;
            printf("T0 : AR <- PC | AR = %03X\n", AR);
            SC++;
        }
        else if (SC == 1) {
            IR = M[AR];
            PC += 1;
            printf("T1 : IR <- M[AR], PC <- PC + 1 | IR = %04X, PC = %03X\n", IR, PC);
            SC++;
        }
        else if (SC == 2) {
            Opcode = (IR & 0x7000) >> 12;
            AR = IR & 0x0FFF;
            I = (IR & 0x8000) >> 15;
            printf("T2 : Decode | Opcode = %d, AR = %03X, I = %d\n", Opcode, AR, I);
            SC++;
        }
        else if (SC == 3) {
            printf("T3 : ");
            if (Opcode == 7) {
                if (I == 0) {
                    if (IR == 0x7800) { AC = 0; printf("Instruction: CLA\n"); }
                    else if (IR == 0x7400) { E = 0; printf("Instruction: CLE\n"); }
                    else if (IR == 0x7200) { AC = (~AC) & 0xFFFF; printf("Instruction: CMA\n"); }
                    else if (IR == 0x7100) { E = E ? 0 : 1; printf("Instruction: CME\n"); }
                    else if (IR == 0x7080) { // CIR
                        int next_E = AC & 1;
                        AC = (AC >> 1) | (E << 15);
                        E = next_E;
                        printf("Instruction: CIR\n");
                    }
                    else if (IR == 0x7040) { // CIL
                        int next_E = (AC >> 15) & 1;
                        AC = ((AC << 1) & 0xFFFF) | E;
                        E = next_E;
                        printf("Instruction: CIL\n");
                    }
                    else if (IR == 0x7020) { AC = (AC + 1) & 0xFFFF; printf("Instruction: INC\n"); }
                    else if (IR == 0x7010) { if ((AC & 0x8000) == 0) PC = (PC + 1) & 0xFFF; printf("Instruction: SPA\n"); }
                    else if (IR == 0x7008) { if ((AC & 0x8000) != 0) PC = (PC + 1) & 0xFFF; printf("Instruction: SNA\n"); }
                    else if (IR == 0x7004) { if (AC == 0) PC = (PC + 1) & 0xFFF; printf("Instruction: SZA\n"); }
                    else if (IR == 0x7002) { if (E == 0) PC = (PC + 1) & 0xFFF; printf("Instruction: SZE\n"); }
                    else if (IR == 0x7001) { printf("Instruction: HLT\n"); break; }
                }
                SC = 0; // 레지스터 참조는 T3에서 완료
            }
            else {
                if (I == 1) {
                    printf("Indirect Address | AR <- M[AR] (Before: %03X, ", AR);
                    AR = M[AR];
                    printf("After: %03X)\n", AR);
                }
                else {
                    printf("Direct Address\n");
                }
                SC++;
            }
        }
        else if (SC == 4 && Opcode != 7) {
            printf("T4 : ");
            DR = M[AR];

            if (Opcode == 0) { // AND
                printf("AND Execution | AC <- AC & DR\n");
                AC = (AC & DR) & 0xFFFF;
                SC = 0;
            }
            else if (Opcode == 2) { // LDA
                printf("LDA Execution | AC <- M[%03X](%04X)\n", AR, DR);
                AC = DR;
                SC = 0;
            }
            else if (Opcode == 1) { // ADD
                printf("ADD Execution | AC(%04X) + DR(%04X)\n", AC, DR);
                AC = (AC + DR) & 0xFFFF; // 16비트 유지
                SC = 0;
            }
            else if (Opcode == 3) { // STA
                printf("STA Execution | M[%03X] <- AC(%04X)\n", AR, AC);
                M[AR] = AC;
                SC = 0;
            }
            else if (Opcode == 4) { // BUN
                printf("BUN Execution | PC <- AR");
                PC = AR;
                SC = 0;
            }
            else if (Opcode == 6) { // ISZ 시작
                printf("ISZ Step 1 | DR <- DR + 1\n");
                DR = (DR + 1) & 0xFFFF; // 가져온 값에 1을 더함[cite: 1]
                SC++; // SC는 5가 됨
            }
            else {
                SC = 0; // 예기치 못한 Opcode 발생 시 무한루프 방지
            }
            printf("Result: AC = %04X, M[%03X] = %04X\n\n", AC, AR, M[AR]);
        }
        else if (SC == 5 && Opcode == 6) { // ISZ의 T5 단계[cite: 1]
            printf("T5 : ISZ Step 2 | M[AR] <- DR\n");
            M[AR] = DR; // 1 증가된 값을 다시 메모리에 저장[cite: 1]
            SC++; // SC는 6이 됨
        }
        else if (SC == 6 && Opcode == 6) { // ISZ의 T6 단계[cite: 1]
            printf("T6 : ISZ Step 3 | If(DR == 0) PC <- PC + 1\n");
            if (DR == 0) { // 결과가 0이면 다음 명령어를 건너뜀[cite: 1]
                PC = (PC + 1) & 0xFFF;
                printf(" >> Result is Zero! Skipping next instruction (PC=%03X)\n", PC);
            }
            SC = 0; // 루프 종료[cite: 1]
        }
    }

    printf("\n--- 최종 프로그램 실행 결과 ---\n");
    for (int i = 0; i < addr; i++) {
        if (M[i] != 0) printf("Address[%03X] : %04X\n", i, M[i]);
    }

    return 0;
}
