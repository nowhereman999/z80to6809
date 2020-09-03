#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void getparts(char line[], char first[], char second[])
{
    int ret,x,pos,pos2;
    
    // Get first part - before comma
    ret = 0;
    for (x = 30; x < 50; x++) {
        if (line[x] == ',') {
            ret = 1;
            pos = x;
        }
    }
    if (ret == 1) {
        strncpy(first, line+30, pos-30);
        first[pos-30] = '\0';
    }
    // get second part - after comma
    ret = 0;
    for (x = pos; x < 50; x++) {
        if (line[x] == ' ') {
            ret = 1;
            pos2 = x;
            x = 50;
        }
    }
    if (ret == 1) {
        strncpy(second, line+pos+1, pos2-pos-1);
        second[pos2-pos-1] = '\0';
    }
}
/*  get single value return with value in first and point indicates:
 0 - register
 1 - points to register mem location [register]
 2 - memory location - $1e00
 3 - points to memory location [$1e00]
 */
int getsingle(char line[], char first[])
{
    int ret,x,pos,test = 0;
    char temp[256];
    int point = 0;
    
    // Get first part - before comma
    ret = 0;
    for (x = 30; x < 50; x++) {
        if (line[x] == ' ') {
            ret = 1;
            pos = x;
            x = 50;
        }
    }
    if (ret == 1) {
        strncpy(first, line+30, pos-30);
        first[pos-30] = '\0';
    }
    if (first[0] == '$') {
        point = 2;
        test = 1;
    }
    if (first[0] == '(') {
        point = 3;
        test = 1;
    }
    if ((strlen(first) == 1) || (strlen(first) == 2)) {
        point = 0;
        test = 1;
        if (ret == 1) {
            strncpy(first, line+30, pos-30);
            first[pos-30] = '\0';
        }
    }
    else {
        strncpy(temp, first, 2);
        temp[2] = '\0';
        // Test for (HL)
        if ((strncmp(temp, "(H" ,2) == 0)) {
            point = 1;
            test = 1;
            strcpy(first, "HL");
        }
    }
    return point;
}

int comparetor (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}


void change(char* var)
{
    //    printf("compare command is %d\n",strcmp(var,"RL"));
    if (strcmp(var, "HL") == 0) {
        strcpy(var,"X");
    }
    if (strcmp(var, "DE") == 0) {
        strcpy(var,"Y");
    }
    if (strcmp(var, "AF") == 0) {
        strcpy(var,"A,CC");
    }
    if (strcmp(var, "BC") == 0) {
        strcpy(var,"U");
    }
//    if (strcmp(var, "H") == 0) {
//        strcpy(var,"A");
//    }
//    if (strcmp(var, "L") == 0) {
//        strcpy(var,"B");
//    }
}

int main(int argc, char* argv[])
{
    FILE *file = NULL;
    char line[256];
    char newcode[256];
    int lnumber, count = 0;
    
    int ret,x,pos,point,found,store = 0;
    char address[256];
    char first[256];
    char second[256];
    char temp[256];
    char oldcode[256];
    char other[256] = "";
    char z80[256];
    char comment[256] = "";
    char m6809[256] = "UNKNOWN";
    char in_out[1000][25];
    
    if (argc == 2)
       	file = fopen(argv[1], "r");
    else {
        fprintf(stderr, "error: wrong number of arguments\n"
                "usage: %s textfile\n", argv[0]);
        return 1;
    }
    
    lnumber = 1;
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
         presence would allow to handle lines longer that sizeof(line) */
        // remove the line feed at the end of 'line'
        line[strcspn(line, "\n")] = 0;

        
        strcpy(m6809,"UNKNOWN");
        strcpy(other,"");
        strcpy(comment,"");
        
        if (strcmp(line, "\n") != 0) {
            strcpy(newcode, "");
        }
        ret = strncmp(line, ";", 1);
        if (ret == 0){
            // find comment
            ret = 0;
            for (x = 0; x < strlen(line); x++) {
                if (line[x] == ';') {
                    ret = 1;
                    pos = x;
                }
            }
            if (ret == 1) {
                strncpy(comment, line+pos, strlen(line)-pos);
                comment[strlen(line)-pos] = '\0';
            }
            strcpy(newcode, comment);
        }
        // line of code
        if ((isdigit(line[0]) == 0)) {
            strcpy(newcode, line);
        }
        else
        {
            // Get oiginal code section
            strncpy(oldcode, line+22, 28);
            oldcode[28] = '\0';
            strncpy(address, line, 4);
            address[4] = '\0';
            // find comment
            ret = 0;
            for (x = 0; x < strlen(line); x++) {
                if (line[x] == ';') {
                    ret = 1;
                    pos = x;
                }
            }
            if (ret == 1) {
                strncpy(comment, line+pos+1, strlen(line)-pos);
                comment[strlen(line)-pos] = '\0';
            }
            // Get 9 characters = mnuemonic for z80
            strncpy(z80, line+22, 9);
            z80[9] = '\0';
            strncpy(temp, line+6, 2);
            temp[2] = '\0';
            //NOP = NOP or 00 data
            if ((strncmp(temp, "00" ,2) == 0)) {
                strcpy(m6809, "FCB     $00");
                //NOP = NOP or 00 data
                if ((strncmp(z80, "NOP " ,4) == 0)) {
                    strcpy(m6809, "NOP ");
                }
            }
            //LD = LDx or STx
            store = 0;
            point = 1;
            strncpy(temp, line+30, 20);
            temp[20] = '\0';
            // check if command is LD
            if ((strncmp(z80, "LD  " ,4) == 0)) {
                // Get first,second values
                //get getparts
                getparts(line, first, second);
                if ((first[0] != '(') && (second[0] != '(') && ((first[0] != '$') && (second[0] != '$'))) {
                    strcpy(m6809, "TFR");
                    strcat(m6809,"     ");
                    change(first);
                    change(second);
                    strcat(m6809,second);
                    strcat(m6809,",");
                    strcat(m6809,first);
                }
                else {
                    strcpy(m6809, "LD");
                    if (second[0] == '$') {
                        strcpy(temp,"#");
                        strcat(temp,second);
                        strcpy(second,temp);
                        point = 0;
                    }
                    if (first[0] == '(') {
                        store = 1;
                        point = 1;
                        strncpy(temp, first+1, strlen(first)-2);
                        temp[strlen(first)-2] = '\0';
                        //reverse order of first, second
                        strcpy(first,second);
                        strcpy(second,temp);
                        strcpy(m6809, "ST");
                    }
                    if (second[0] == '(') {
                        point = 1;
                        strncpy(temp, second+1, strlen(second)-2);
                        temp[strlen(second)-2] = '\0';
                        strcpy(second, temp);
                    }
                    if (point == 1) {
                        strcpy(temp, "");
                        if (strlen(second) < 3) {
                            strcat(temp,",");
                        }
                        change(second);
                        strcat(temp, second);
                        strcat(temp, "");
                        strcpy(second, temp);
                    }
                    change(first);
                    strcat(m6809,first);
                    strcat(m6809,"     ");
                    strcat(m6809,second);
                    if ((strncmp(m6809, "ST#$00     ,X" ,13) == 0)) {
                        strcpy(m6809, "CLR     ,X");
                    }
                    if ((strncmp(m6809, "ST#$00     ,Y" ,13) == 0)) {
                        strcpy(m6809, "CLR     ,Y");
                    }
                    
                }
            }
            //JP = JMP
            if ((strncmp(z80, "JP  " ,4) == 0)) {
                strncpy(temp, line+6, 2);
                temp[2] = '\0';
                if ((strncmp(temp, "C3" ,2) == 0)) {
                    strcpy(m6809, "LBRA    ");
                    point = getsingle(line, first);
                    strcat(m6809,first);
                }
                else {
                    //get getparts
                    getparts(line, first, second);
                    if (strlen(first) == 2) {
                        if ((strncmp(first, "NZ" ,2) == 0)) {
                            strcpy(m6809, "LBNE    ");
                        }
                        if ((strncmp(first, "NC" ,2) == 0)) {
                            strcpy(m6809, "LBCC    ");
                        }
                    }
                    if ((strncmp(first, "C" ,1) == 0)) {
                        strcpy(m6809, "LBCS    ");
                    }
                    if ((strncmp(first, "Z" ,1) == 0)) {
                        strcpy(m6809, "LBEQ    ");
                    }
                    if ((strncmp(first, "M" ,1) == 0)) {
                        strcpy(m6809, "LBMI    ");
                    }
                    strcat(m6809,second);
                }
            }
            //DEC = DEC
            if ((strncmp(z80, "DEC " ,4) == 0)) {
                // Get value
                point = getsingle(line, first);
                // 0 - register
                // 2 - memory location - $1e00
                // 3 - points to memory location [$1e00]
                if ((point == 0) || (point == 2) || (point == 3)) {
                    strcpy(temp, "DEC     ");
                    if (strlen(first) < 3) {
                        strcat(temp,"");
                    }
                    change(first);
                    if ((strncmp(first, "A" ,1) == 0) || (strncmp(first, "B" ,1) == 0)) {
                        strcpy(m6809, "DEC");
                        strcat(m6809,first);
                        strcat(m6809,"    ");
                    }
                    else {
                    strcat(temp, first);
                    strcpy(m6809, temp);
                    }
                }
                // 1 - points to register mem location [register]
                if (point == 1) {
                    strcpy(temp, "DEC      ");
                    if (strlen(first) < 3) {
                        strcat(temp,",");
                    }
                    change(first);
                    strcat(temp, first);
                    strcat(temp, " ");
                    strcpy(m6809, temp);
                }
                if ((strncmp(m6809, "DEC     X" ,9) == 0)) {
                    strcpy(m6809, "LEAX    -1,X");
                }
                if ((strncmp(m6809, "DEC     Y" ,9) == 0)) {
                    strcpy(m6809, "LEAY    -1,Y");
                }
            }
            //INC = INC
            if ((strncmp(z80, "INC " ,4) == 0)) {
                // Get value
                point = getsingle(line, first);
                // 0 - register
                // 2 - memory location - $1e00
                // 3 - points to memory location [$1e00]
                if ((point == 0) || (point == 2) || (point == 3)) {
                    strcpy(temp,"INC     ");
                    if (strlen(first) < 3) {
                        strcat(temp,"");
                    }
                    change(first);
                    if ((strncmp(first, "A" ,1) == 0) || (strncmp(first, "B" ,1) == 0)) {
                        strcpy(m6809, "INC");
                        strcat(m6809,first);
                        strcat(m6809,"    ");
                    }
                    else {
                        strcat(temp, first);
                        strcpy(m6809, temp);
                    }
                }
                // 1 - points to register mem location [register]
                if (point == 1) {
                    strcpy(temp, "INC      ");
                    if (strlen(first) < 3) {
                        strcat(temp,",");
                    }
                    change(first);
                    strcat(temp, first);
                    strcat(temp, " ");
                    strcpy(m6809, temp);
                }
                if ((strncmp(m6809, "INC     X" ,9) == 0)) {
                    strcpy(m6809, "LEAX    1,X");
                }
                if ((strncmp(m6809, "INC     Y" ,9) == 0)) {
                    strcpy(m6809, "LEAY    1,Y");
                }
            }
            //RAR = Rotate Right, bit 0 is Copied to Carry old Carry bit to bit 7
            if ((strncmp(z80, "RRA " ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"RORA");
            }
            //RLA = Rotate Left, bit 7 is Copied to Carry old Carry bit to bit 0
            if ((strncmp(z80, "RLA " ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"ROLA");
            }        //ADD = ADD
            if ((strncmp(z80, "ADD " ,4) == 0)) {
                point = 0;
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"ADD     ");
                change(first);
                strcat(m6809,first);
                strcat(m6809,"=");
                strcat(m6809,first);
                strcat(m6809,"+");
                if (second[0] == '(') {
//                    point = 1;
                    strncpy(temp, second+1, strlen(second)-2);
                    temp[strlen(second)-2] = '\0';
                    strcpy(second, temp);
                }
                if (point == 1) {
                    strcpy(temp, "[");
                    change(second);
                    strcat(temp, second);
                    strcat(temp, "]");
                    strcpy(second, temp);
                }
                else {
                    change(second);
                    strcat(m6809,second);
                }
                if ((strncmp(m6809, "ADD     A=A+" ,12) == 0)) {
                    strcpy(m6809, "ADDA    ,");
                    change(second);
                    strcat(m6809,second);
                    if (second[0] == '$') {
                        strcpy(m6809, "ADDA    #");
                        strcat(m6809,second);
                    }
                }
            }
            //AND = AND
            if ((strncmp(z80, "AND " ,4) == 0)) {
                // Get value
                point = getsingle(line, first);
                // 0 - register
                // 3 - points to memory location [$1e00]
                if ((point == 0) ||  (point == 3)) {
                    strcpy(m6809,"ANDA    ");
                    if (strncmp(first, "A", 1) == 0) {
                        strcpy(m6809,"TSTA");
                    }
                    else {
                        strcat(m6809,first);
                    }
                }
                // 2 - value - $A00
                if (point == 2) {
                    strcpy(m6809,"ANDA    #");
                    strcat(m6809,first);
                }
                // 1 - points to register mem location [register]
                if (point == 1) {
                    strcpy(m6809,"ANDA     ,");
                    change(first);
                    strcat(m6809,first);
                }
            }
            //CP = subtraction from A but doesn't modify A
            //Work out similar function with CMPA - Compare A 6809
            if ((strncmp(z80, "CP  " ,4) == 0)) {
                // Get value
                point = getsingle(line, first);
                // 0 - register
                // 3 - points to memory location [$1e00]
                if ((point == 0) || (point == 3)) {
                    strcpy(m6809,"CMPA    ");
                    strcat(m6809,first);
                    strcat(m6809,"_Temp");
                }
                // 2 - value - $A00
                if (point == 2) {
                    strcpy(m6809,"CMPA    #");
                    strcat(m6809,first);
                }
                // 1 - points to register mem location register
                if (point == 1) {
                    strcpy(m6809,"CMPA     ,");
                    change(first);
                    strcat(m6809,first);
                }
            }
            //SBI = Subtract with borrow A <- A - byte - Carry
            if ((strncmp(z80, "SBI " ,4) == 0)) {
                point = 0;
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"SBCA     #");
                strcat(m6809,second);
            }
            //SUI = Subtract Immediate from accumulator
            if ((strncmp(z80, "SUI " ,4) == 0)) {
                point = 0;
                //get getparts
                point = getsingle(line, first);
                strcpy(m6809,"SUBA    #");
                strcat(m6809,first);
            }
            //SUB  A = subtract A from A - same as 6809 CLRA
            if ((strncmp(z80, "SUB     A" ,4) == 0)) {
                strcpy(m6809, "CLRA");
            }
            //SCF = Set Carry Flag
            if ((strncmp(z80, "SCF " ,4) == 0)) {
                strcpy(m6809,"ORCC     #$01");
            }
            //CPL = Compliment the Accumulator
            if ((strncmp(z80, "CPL " ,4) == 0)) {
                strcpy(m6809,"COMA         ");
            }
            //XOR = XORS A with A or with Registor or memory
            if ((strncmp(z80, "XOR " ,4) == 0)) {
                strcpy(m6809, "EORA    ");
                // Get value
                point = getsingle(line, first);
                strcat(m6809,first);
                if ((strncmp(m6809, "EORA    A" ,9) == 0)) {
                    strcpy(m6809,"CLRA         ");
                }
            }
            // Instructions that need more lines of code to work correctly are below...
            //IN = Read value from hardware port into A
            if ((strncmp(z80, "IN  " ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"LDA     ");
                strncpy(z80, line+9, 2);
                z80[2] = '\0';
                strcat(m6809,"IO_");
                strcat(m6809,z80);
                strcat(m6809,"_");
                if (second[0] == '(') {
                    strncpy(temp, second+1, strlen(second)-2);
                    temp[strlen(second)-2] = '\0';
                    strcpy(second, temp);
                }
                strcat(m6809,second);
                // Let's see if it's new and if so add it to the bottom of the table
                found = 0;
                strncpy(temp, m6809+8, strlen(m6809)-8);
                temp[strlen(m6809)-8] = '\0';
                for (x = 0; x < count ; x++) {
                    if (strcmp(temp,in_out[x]) == 0) {
                        found = 1;
                        x = count;
                    }
                }
                if (found == 0) {
                    //               printf("Adding-%s\n",temp);
                    strcpy(in_out[count],temp);
                    count++;
                }
            }
            //OUT = Store value to hardware port from A
            if ((strncmp(z80, "OUT " ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"STA     ");
                strncpy(z80, line+9, 2);
                z80[2] = '\0';
                strcat(m6809,"IO_");
                strcat(m6809,z80);
                strcat(m6809,"_");
                if (first[0] == '(') {
                    strncpy(temp, first+1, strlen(first)-2);
                    temp[strlen(first)-2] = '\0';
                    strcpy(first, temp);
                }
                strcat(m6809,first);
                // Let's see if it's new and if so add it to the bottom of the table
                found = 0;
                strncpy(temp, m6809+8, strlen(m6809)-8);
                temp[strlen(m6809)-8] = '\0';
                for (x = 0; x < count ; x++) {
                    if (strcmp(temp,in_out[x]) == 0) {
                        found = 1;
                        x = count;
                    }
                }
                if (found == 0) {
                    //               printf("Adding-%s\n",temp);
                    strcpy(in_out[count],temp);
                    count++;
                }
            }
            //DAA = convert the A register into two BCD values 4 bits each
            if ((strncmp(z80, "DAA " ,4) == 0)) {
                strcpy(m6809, "DAA     ");
                strncpy(z80, line+30, 2);
                z80[2] = '\0';
                strcat(m6809,z80);
            }
            //CALL = LBSR
            if ((strncmp(z80, "CALL" ,4) == 0)) {
                strncpy(temp, line+6, 2);
                temp[2] = '\0';
                if ((strncmp(temp, "CD" ,2) == 0)) {
                    strcpy(m6809, "LBSR    ");
                    point = getsingle(line, first);
                    strcat(m6809,first);
                }
                else {
                    //get getparts
                    getparts(line, first, second);
                    if (strlen(first) == 2) {
                        if ((strncmp(first, "NZ" ,2) == 0)) {   // if Z = 0
                            x = (int)strtol(address, NULL, 16);
                            x = x + 3;
                            sprintf(temp, "%04X", x);
                            strcpy(other, "        BEQ     $");
                            strcat(other,temp);
                            strcat(other,"                             ;");
                            strcat(other,address);
                            strcat(other," -");
                            strcat(other,comment);
                            strcat(other,"\n");
                            strcpy(address,"****");
                            strcpy(comment," Added for 6809 worakaround");
                            strcpy(m6809, "LBSR    ");
                        }
                        if ((strncmp(first, "NC" ,2) == 0)) {   // if C = 0
                            x = (int)strtol(address, NULL, 16);
                            x = x + 3;
                            sprintf(temp, "%04X", x);
                            strcpy(other, "        BCS     $");
                            strcat(other,temp);
                            strcat(other,"                             ;");
                            strcat(other,address);
                            strcat(other," -");
                            strcat(other,comment);
                            strcat(other,"\n");
                            strcpy(address,"****");
                            strcpy(comment," Added for 6809 worakaround");
                            strcpy(m6809, "LBSR    ");
                        }
                    }
                    if ((strncmp(first, "C" ,1) == 0)) {   // if C = 1
                        x = (int)strtol(address, NULL, 16);
                        x = x + 3;
                        sprintf(temp, "%04X", x);
                        strcpy(other, "        BCC     $");
                        strcat(other,temp);
                        strcat(other,"                             ;");
                        strcat(other,address);
                        strcat(other," -");
                        strcat(other,comment);
                        strcat(other,"\n");
                        strcpy(address,"****");
                        strcpy(comment," Added for 6809 worakaround");
                        strcpy(m6809, "LBSR    ");
                    }
                    if ((strncmp(first, "Z" ,1) == 0)) {   // if Z = 1
                        x = (int)strtol(address, NULL, 16);
                        x = x + 3;
                        sprintf(temp, "%04X", x);
                        strcpy(other, "        BNE     $");
                        strcat(other,temp);
                        strcat(other,"                             ;");
                        strcat(other,address);
                        strcat(other," -");
                        strcat(other,comment);
                        strcat(other,"\n");
                        strcpy(address,"****");
                        strcpy(comment," Added for 6809 worakaround");
                        strcpy(m6809, "LBSR    ");
                    }
                    //                if ((strncmp(first, "M" ,1) == 0)) {   // if M = 1
                    //                    strcpy(m6809, "LBSR based on LBMI    ");
                    //                }
                    strcat(m6809,second);
                }
            }
            //PUSH = PSHS
            if ((strncmp(z80, "PUSH" ,4) == 0)) {
                strcpy(m6809, "PSHS    ");
                strncpy(z80, line+30, 2);
                z80[2] = '\0';
                change(z80);
                strcat(m6809,z80);
            }
            //POP = PULS
            if ((strncmp(z80, "POP " ,4) == 0)) {
                strcpy(m6809, "PULS    ");
                strncpy(z80, line+30, 2);
                z80[2] = '\0';
                change(z80);
                strcat(m6809,z80);
            }
            //EI = Enable Interrupts
            if ((strncmp(z80, "EI  " ,4) == 0)) {
                strcpy(m6809, "ANDCC    #$AF");
                strncpy(z80, line+30, 2);
                z80[2] = '\0';
                strcat(m6809,z80);
            }
            //RET = Return from subroutine or Return from Interrupt
            if ((strncmp(z80, "RET " ,4) == 0)) {
                strncpy(temp, line+6, 2);
                temp[2] = '\0';
                if ((strncmp(temp, "C9" ,2) == 0)) {
                    strcpy(m6809, "RTS     ");
                    point = getsingle(line, first);
                    strcat(m6809,first);
                }
                else {
                    // Get value
                    point = getsingle(line, first);
                    if (strlen(first) == 2) {
                        if ((strncmp(first, "PE" ,2) == 0)) {
                            strcpy(m6809, "RTS - If PE is set    ");
                        }
                        if ((strncmp(first, "NZ" ,2) == 0)) {
                            strcpy(m6809, "RTS - If NZ is set BNE to RTS address  ");
                        }
                        if ((strncmp(first, "NC" ,2) == 0)) {
                            strcpy(m6809, "RTS - If NC is set BCC to RTS address  ");
                        }
                    }
                    if ((strncmp(first, "C" ,1) == 0)) {
                        strcpy(m6809, "RTS - If C is set BCS to RTS address  ");
                    }
                    if ((strncmp(first, "M" ,1) == 0)) {
                        strcpy(m6809, "RTS - If M is set    ");
                    }
                    if ((strncmp(first, "Z" ,1) == 0)) {
                        strcpy(m6809, "RTS - If Z is set BEQ to RTS address  ");
                    }
                }
            }
            //EX = Exchange registers or memory pointed to
            if ((strncmp(z80, "EX  " ,4) == 0)) {
                point = 0;
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"EXG     ");
                change(first);
                strcat(m6809,first);
                strcat(m6809,",");
                change(second);
                strcat(m6809,second);
            }
            //OR = ORA with operand
            if ((strncmp(z80, "OR  " ,4) == 0)) {
                // Get value
                point = getsingle(line, first);
                // 0 - register
                if (point == 0) {
                    strcpy(m6809,"ORA   Register - need tweaking  ");
                    strcat(m6809,first);
                }
                // 2 - value - $A00
                if (point == 2) {
                    strcpy(m6809,"ORA     #");
                    strcat(m6809,first);
                }
                // 1 - points to register mem location [register]
                // 3 - points to memory location [$1e00]
                if (point == 1) {
                    strcpy(m6809,"ORA      ,");
                    change(first);
                    strcat(m6809,first);
                }
                if (point == 3) {
                    strcpy(m6809,"ORA      ");
                    strcat(m6809,first);
                }
            }
            //PCHL = Load Program Counter , TFR HL,PC
            if ((strncmp(z80, "PCHL" ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"TFR     HL,PC  transfer HL to Program counter - Fix");
                strcat(m6809,second);
            }
            //ADC = ADD two operands plus the carry flag store it back into first operand
            if ((strncmp(z80, "ADC " ,4) == 0)) {
                point = 0;
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"ADC     ");
                strcat(m6809,first);
                strcat(m6809,",");
                strcat(m6809,second);
                strcat(m6809," - Fix");
            }
            //RRCA = Rotate Right, Copy bit 0 to Carry, Copy bit 0 to bit 1
            if ((strncmp(z80, "RRCA" ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"RORA");
            }
            //RLCA = Rotate Left, Copy bit 7 to Carry, Copy bit 7 to bit 0
            if ((strncmp(z80, "RLCA" ,4) == 0)) {
                //get getparts
                getparts(line, first, second);
                strcpy(m6809,"ROLA");
            }
            
            if ((strncmp(m6809, "UNKNOWN" ,7) == 0)) {
                printf("error: unknown instruction -%s-\n"
                       "address: %s\n"
                       "line: %s\n", z80, address,line);
                exit(-2);
            }
            
            strcpy(temp,"        ");
            strcat(temp,m6809);
            strcpy(m6809,temp);
            strcat(m6809,"                                                          ");
            strncpy(temp, m6809, 50);
            temp[50] = '\0';
            strcpy(m6809,temp);
            strcpy(temp,";");
            strcat(m6809,temp);
            strcat(other,m6809);
            strcpy(m6809,other);
            strcat(m6809,address);
            strcat(m6809," -");
            strcat(m6809,oldcode);
            strcat(m6809,comment);
            strcat(newcode,m6809);
        }
        
        //        printf("%s\n%s\n", line, newcode);
        printf("%s\n", newcode);
        lnumber++;
        
    }
    printf("; IN/OUT pointers (amnually have to set):\n");
    for (x = 0; x < count ; x++) {
        printf("%-20sFCB     $00\n",in_out[x]);
    }
   
    fclose(file);
    
    return 0;
}
