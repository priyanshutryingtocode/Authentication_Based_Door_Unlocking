#include<reg52.h>
#include<stdio.h> 

// PINS 
sbit r0=P2^0; sbit r1=P2^1; sbit r2=P2^2; sbit r3=P2^3;
sbit c0=P2^5; sbit c1=P2^6; sbit c2=P2^7;
sbit en=P3^6; sbit rs=P3^5; sbit rw=P3^7; 
sbit led_pin = P3^1;   

// PASSWORDS
char main_pass[]    = ""; 
char privacy_code[] = ""; 

// FLAGS & BUFFERS
char input_buffer[17];         
unsigned char input_index = 0; 
unsigned char wrong_attempts = 0; 

volatile char uart_command = 0; 
unsigned char privacy_mode = 0; 

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++) for (j = 0; j < 123; j++); 
}

// UART
void UART_Init() {
    TMOD |= 0x20; TH1 = 0xFD; SCON = 0x50; TR1 = 1; ES = 1; EA = 1;         
}

// ISR
void Serial_ISR() interrupt 4 {
    if (RI) {
        char incoming = SBUF; 
        RI = 0; 
        
    
        if (privacy_mode == 1 && incoming == '1') {
          
        } else {
            uart_command = incoming; 
        }
    }
}

void lcdcmd(unsigned char command) {
    ES = 0; P1 = command; rs = 0; rw = 0; en = 1; delay_ms(2); en = 0; delay_ms(2); ES = 1;       
}
void lcddata(char data1) {
    ES = 0; P1 = data1; rs = 1; rw = 0; en = 1; delay_ms(2); en = 0; delay_ms(2); ES = 1;       
}
void lcdint() {
    delay_ms(20); lcdcmd(0x38); delay_ms(5); lcdcmd(0x38); delay_ms(1); lcdcmd(0x38); 
    lcdcmd(0x38); lcdcmd(0x0C); lcdcmd(0x01); delay_ms(2); lcdcmd(0x06); lcdcmd(0x80); 
}
void lcd_display_string(char *str) {
    while(*str != '\0') { lcddata(*str); str++; }
}


int check_password(char *input) {
    unsigned char i;
    int match;

    match = 1; 
    for(i=0; i<6; i++) if(input[i]!=main_pass[i]) match=0;
    if(match) return 1;

    match = 1; 
    for(i=0; i<6; i++) if(input[i]!=privacy_code[i]) match=0;
    if(match) return 2;

    return 0; 
}

char getKey() {
    r0=0; r1=1; r2=1; r3=1; if(!c0){delay_ms(20);while(!c0);return '1';} if(!c1){delay_ms(20);while(!c1);return '2';} if(!c2){delay_ms(20);while(!c2);return '3';}
    r0=1; r1=0; r2=1; r3=1; if(!c0){delay_ms(20);while(!c0);return '4';} if(!c1){delay_ms(20);while(!c1);return '5';} if(!c2){delay_ms(20);while(!c2);return '6';}
    r0=1; r1=1; r2=0; r3=1; if(!c0){delay_ms(20);while(!c0);return '7';} if(!c1){delay_ms(20);while(!c1);return '8';} if(!c2){delay_ms(20);while(!c2);return '9';}
    r0=1; r1=1; r2=1; r3=0; if(!c0){delay_ms(20);while(!c0);return '*';} if(!c1){delay_ms(20);while(!c1);return '0';} if(!c2){delay_ms(20);while(!c2);return '#';}
    return 0; 
}

void reset_prompt() {
    input_index = 0; 
    lcdcmd(0x01); 
    
    if(privacy_mode == 1) {
        lcd_display_string("Privacy Active");
        lcdcmd(0xC0);
        lcd_display_string("App Blocked");
    } else {
        lcd_display_string("Enter Password:");
        lcdcmd(0xC0);
    }
}

void action_unlock(char* msg) {
    lcdcmd(0x01); lcd_display_string(msg); 
    led_pin = 0; wrong_attempts = 0; delay_ms(5000);         
    led_pin = 1; lcdcmd(0x01); lcd_display_string("Locked"); delay_ms(2000);   
    reset_prompt();
}

void action_lockout() {
    char timer_buffer[10]; int i, j;
    lcdcmd(0x01); lcd_display_string("System Locked!"); delay_ms(1000);
    for (i = 30; i > 0; i--) {
        lcdcmd(0x01); lcd_display_string("Try Again in:"); lcdcmd(0xC0); 
        sprintf(timer_buffer, "%d sec", i); lcd_display_string(timer_buffer);
        
        for(j=0; j<20; j++) {
            delay_ms(50);
            if (uart_command == '1') return; 
        }
    }
    wrong_attempts = 0; 
}

void action_wrong() {
    wrong_attempts++; 
    lcdcmd(0x01); lcd_display_string("Wrong Password"); led_pin = 1; delay_ms(1500);
    if (wrong_attempts >= 3) {
        action_lockout(); 
			
        if (uart_command == '1') {
            action_unlock("Remote Override"); uart_command = 0; return;
        }
    }
    reset_prompt();
}


void main() {
    char key; 

    int result; 

    P1 = 0x00; P3 = 0xFF; P2 = 0xF0; led_pin = 1;     
    delay_ms(500); lcdint(); reset_prompt(); UART_Init();    

    while(1) {
        key = getKey(); 
        
        if (uart_command != 0) {
            char cmd = uart_command;
            uart_command = 0; 
            
            if (cmd == '1') action_unlock("Remote Unlocked");
            else if (cmd == '0') action_wrong();
        }

        if(key != 0) {
            if(key == '#') {
                input_buffer[input_index] = '\0'; 
                
     
                result = check_password(input_buffer);

                if(result == 1) {       
             
                    action_unlock("Unlocked");
                }
                else if(result == 2) {  
                 
                    if(privacy_mode == 0) {
                        privacy_mode = 1;
                        lcdcmd(0x01); lcd_display_string("Privacy ON");
                    } else {
                        privacy_mode = 0;
                        lcdcmd(0x01); lcd_display_string("Privacy OFF");
                    }
                    delay_ms(2000);
                    reset_prompt();
                }
                else {
                    action_wrong();
                }
            }
            else if(key == '*') {
                if(input_index > 0) {
                    input_index--; lcdcmd(0x10); lcddata(' '); lcdcmd(0x10);          
                }
            }
            else {
                if(input_index < 16) {
                    input_buffer[input_index] = key; input_index++; lcddata('*');                  
                }
            }
        }
    }
}
