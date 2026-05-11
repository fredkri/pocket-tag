#include "debug.h"
#define DEBUG_BOARD

#define I2C_ADDR 0x17

// ============ FUNCTION HEADERS =============
void flash();
void send_test();
void send_pew();
void storage_save_id();
void storage_save_name();
void storage_save_hit_song();
void buzzer_sweep(uint16_t start_freq, uint16_t end_freq, uint16_t duration_ms);


// =========== GLOBAL VARIABLES ==============
volatile uint8_t player_id = 0;
volatile uint8_t player_name[17];
volatile uint8_t hit_song[17];

// =========== I2C command IDs ===============
#define I2C_COMMAND_ID      0x01
#define I2C_COMMAND_NAME    0x02
#define I2C_COMMAND_HITSONG 0x03
#define I2C_COMMAND_NOTE    0x04


// =================================================================================
// =============================== GPIOS and HW CONFIG =============================
// =================================================================================

void init_hardware(){
    // === Configure pins ===
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    // D1 as input from button (Input Pull Up)
    // Note that D1 is also the programming "DIO" pin, therefore:
    // When set as input, Debugging will not work
    // When set as output, Debbugging NOR PROGRAMMING will not work, so don't do that
    #ifndef DEBUG_BOARD
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
    #endif

    //D6 as input from IR receiver
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // A2 as Lights and Buzzer (Output Push-Pull)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // ACTIVE LOW because PMOS, so we set it HIGH to start with
    GPIO_SetBits(GPIOA, GPIO_Pin_2);

    // C4 as output to IR LED (Output Push-Pull)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    // ACTIVE LOW because PMOS, so we set it HIGH to start with
    GPIO_SetBits(GPIOC, GPIO_Pin_4);


    // ====== Pin config for debugging prototype on breadboard ==========

    // D3 as Button input on prototype to not have to use D1 which breaks debugging
    #ifdef DEBUG_BOARD
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
   

        //D4 as LED output for debug?
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        //D5 as UART TX
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        USART_InitTypeDef USART_InitStructure = {0};
        USART_InitStructure.USART_BaudRate = 230400; // Common IR speed
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Tx;
        USART_Init(USART1, &USART_InitStructure);
        USART_Cmd(USART1, ENABLE);
     #endif

    // xxxxxxxx  end of config for prototype on breadboard  xxxxxxxxxx

    // ==== TIM2 timer for timing the IR pulses when receiving ====
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    TIM_TimeBaseInitStructure.TIM_Prescaler = 24 - 1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    TIM_Cmd(TIM2, ENABLE);

    // === I2C on pins PC1(SDA) and PC2(SCL) ====
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    I2C_DeInit(I2C1);
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_ADDR << 1;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);

    // Enable interrupts
    I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);

    // Enable NVIC interrupt
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);
}


// =================== GPIO wrappers ================

// TRIGGER BUTTON on D1 (or D3 on the prototype)
uint16_t io_trigger_pressed(){
    #ifndef DEBUG_BOARD
        return !(GPIO_ReadInputData(GPIOD) & GPIO_Pin_1);
    #else
        return !(GPIO_ReadInputData(GPIOD) & GPIO_Pin_3);
    #endif
}

void io_buzzflasher(uint8_t state){
    // Note that its active LOW because controled over PMOS
    if(state){
        GPIO_ResetBits(GPIOA, GPIO_Pin_2);
        // also the debug LED on D4
        GPIO_SetBits(GPIOD, GPIO_Pin_4);
    }else{
        GPIO_SetBits(GPIOA, GPIO_Pin_2);
        // also the debug LED on D4
        GPIO_ResetBits(GPIOD, GPIO_Pin_4);
    }
}

// =================================================================================
// ================================ IR SHOOTING DATA ===============================
// =================================================================================
/* 
    Transmit 1s and 0s using the GAP LENGTH MODULATION
    Pulses are seen as LOW on the RX pin, and are always 12 long
    Gaps are seen as HIGH, and are 14 for a "zero" and 18 for a "one"
*/

// ============ IR TX ============

// - lowest level: send PULSES and GAPS of given length
void ir_pulse(uint16_t length){
    while(length){
        GPIO_ResetBits(GPIOC, GPIO_Pin_4);
        Delay_Us(8);
        GPIO_SetBits(GPIOC, GPIO_Pin_4);
        Delay_Us(8);
        length--;
    }
}

void ir_gap(uint16_t length){
    while(length){
        GPIO_SetBits(GPIOC, GPIO_Pin_4);
        Delay_Us(8);
        GPIO_SetBits(GPIOC, GPIO_Pin_4);
        Delay_Us(8);
        length--;
    }
}

// - one level up: send ONES, ZEROS and some special contorl signals using the pulses and gaps
void ir_send_one(){
    ir_gap(28);
    ir_pulse(12);
}

void ir_send_zero(){
    ir_gap(14);
    ir_pulse(12);
}

void ir_send_preamble(){
    ir_pulse(228);
    ir_gap(228);
}

void ir_send_starting_pulse(){
    ir_pulse(12);
}

void ir_send_ending_pulse(){
    ir_pulse(12);
}
void ir_send_break_after_byte(){
    ir_gap(43);
}

// - two levels up: send a byte worth of data using the ONES, ZEROS and control signals
void send_byte(uint8_t byte){
    // start pulse
    ir_send_starting_pulse();
    
    // 8 bit payload
    for(int i=8; i; i--){
        if(byte & 0x80){
            ir_send_one();
        }else{
            ir_send_zero();
        }
        byte<<=1;
    }
    ir_send_break_after_byte();
}


// ========= IR RX ========

// lowest level: get the length of a gap
uint16_t get_gap(){

    // if already high, abort:
    if(GPIO_ReadInputData(GPIOD) & GPIO_Pin_6){return 0xFFFF;}

    // wait to go HIGH
    TIM2->CNT = 0;
    while(!(GPIO_ReadInputData(GPIOD) & GPIO_Pin_6)){if(TIM2->CNT > 10000){return 0xFFFF;}}
    TIM2->CNT = 0;
    // wait to go LOW again
    while(GPIO_ReadInputData(GPIOD) & GPIO_Pin_6){if(TIM2->CNT > 10000){return 0xFFFF;}}

    // return lenght of gap betweeen pulses
    return TIM2->CNT;
}

// top level: receive entire bytes
// recode_rx() returns "1" when a new byte is available (otherwise "0")
// the new byte is stored in "rx_data".
uint16_t rx_data;
uint16_t decode_rx(){
    uint16_t time;
    
    static uint16_t state = 1;
    static uint16_t data = 0;
    static uint16_t bits_received = 0;
   
    time = get_gap();
    
    // scale because;
    time >>= 4;

    // on timeout, reset state
    if(time == 0xFFFF){
        state = 0;
        rx_data = 0;
        return 0;
    }
    
    switch(state){
        // // waiting for preamble. It can come at any time so we just assume its good
        // case 0:  
        //     state = 1;
        //     break;
        
        // preamble length. Not sure yet so assume its good
        case 1:
            if(time > 250 && time < 500){
                state = 2;
                bits_received = 0;
                data = 0;
            }
            break;
        
        // actual bits
        case 2:
            if(time > 60 || time < 10){
                // sorry! too long or too short!
                state = 1;
                break;
            }
            
            if(time > 30 && time < 50){
                // looks like a 1!
                data |= 1;
            }

            // count the new bit, see if we have a byte
            bits_received += 1;
            // USART_SendData(USART1, '0' + bits_received);
            if(bits_received >= 8){
                state = 3;
            }else{
                data <<= 1;
            }
            break;
        
            // end symbol
        case 3:
            if(time> 50 && time < 80){
                // end symbol valid! the data is valid!
                state = 2;
                rx_data = data;
                data = 0;
                bits_received = 0;
                // USART_SendData(USART1, 'K');
                return 1;
               
            }
            // otherwise, reset to preamble listen
            state = 1;
            break;
    }
    return 0;
}

// ================================= I2C ===================
volatile uint8_t i2c_command[64];
volatile uint16_t i2c_rx_index = 0;
volatile uint16_t i2c_tx_index = 0;
volatile uint8_t* i2c_tx_pointer = 0;


// I2C_COMMAND_ID
void i2c_received_id(){
    if(i2c_command[1] == 'I' && i2c_command[2] == 'D'){
        player_id = i2c_command[3];
    }
    storage_save_id();
}

// I2C_COMMAND_NAME
void i2c_received_name(){
    // copy the new name
    for(int i=0; i<i2c_rx_index; i++){
        player_name[i] = i2c_command[i+1];
    }
    // and fill the rest with zeros
    for(int i=i2c_rx_index; i<16; i++){
        player_name[i] = '\0';
    }
    storage_save_name();
}

// I2C_COMMAND_HITSONG
void i2c_received_hitsong(){
 // copy the new name
    for(int i=0; i<i2c_rx_index; i++){
        hit_song[i] = i2c_command[i+1];
    }
    // and fill the rest with zeros
    for(int i=i2c_rx_index; i<16; i++){
        hit_song[i] = '\0';
    }
    storage_save_hit_song();
}


// I2C_COMMAND_NOTE
void i2c_play_note(){
    buzzer_sweep(800, 400, 100);
}

void i2c_parse_command(){
    switch(i2c_command[0]){
        case I2C_COMMAND_ID:
            if(i2c_rx_index == 1){
                i2c_tx_pointer = &player_id;
            }else{
                i2c_received_id();
            }
            break;
        
        case I2C_COMMAND_NAME:
            if(i2c_rx_index == 1){
                i2c_tx_pointer = &player_name[0];
            }else{
                i2c_received_name();
            }
            break;
        
        case I2C_COMMAND_HITSONG:
            if(i2c_rx_index == 1){
                i2c_tx_pointer = &hit_song[0];
            }else{
                i2c_received_hitsong();
            }
            break;

        case I2C_COMMAND_NOTE:
            if(i2c_rx_index > 1){
                i2c_play_note();
            }
            break;
    
        default:
            break;
    }
    // clear the command type, so it is not accidentally executed again
    i2c_command[0] = 0;
}



// EVENT INTERRUPT
void I2C1_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_EV_IRQHandler(void){
    uint32_t event;

    event = I2C_GetLastEvent(I2C1);

    switch(event){
        // Address matched, master writing to us
        case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:{
            i2c_rx_index = 0;
            break;
        }

        case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:{
            i2c_tx_index = 0;
            break;
        }

        // Byte received
        case I2C_EVENT_SLAVE_BYTE_RECEIVED:{
            // put new byte in command buffer
            if(i2c_rx_index<64){
                 i2c_command[i2c_rx_index] = I2C_ReceiveData(I2C1);
            }
            i2c_rx_index++;
            break;
        }

        case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:
        case I2C_EVENT_SLAVE_BYTE_TRANSMITTING:{
            if(i2c_tx_index < 64){
                I2C_SendData(I2C1, i2c_tx_pointer[i2c_tx_index]);
                i2c_tx_index += 1;
            }else{
                I2C_SendData(I2C1, 0x00);
            }
            break;
        }


        // STOP condition from master
        case I2C_EVENT_SLAVE_STOP_DETECTED:{
            // Clear STOPF:
            // read STAR1 then write CTLR1
            (void)I2C1->STAR1;
            I2C1->CTLR1 |= I2C_CTLR1_PE;

            // and now, parse what we got.
            // this can be potentialy pulled out of the interrupt maybe
            i2c_parse_command();
            break;
        }

        default:
            break;
    }
}


// ERROR INTERRUPT
void I2C1_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_ER_IRQHandler(void){
    // Clear common error flags

    if(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF)){
        I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    }

    if(I2C_GetFlagStatus(I2C1, I2C_FLAG_BERR)){
        I2C_ClearFlag(I2C1, I2C_FLAG_BERR);
    }

    if(I2C_GetFlagStatus(I2C1, I2C_FLAG_ARLO)){
        I2C_ClearFlag(I2C1, I2C_FLAG_ARLO);
    }

    if(I2C_GetFlagStatus(I2C1, I2C_FLAG_OVR)){
        I2C_ClearFlag(I2C1, I2C_FLAG_OVR);
    }
}

// ========= Buzzer ========

// beep with buzzer (also shitty implementation using delay)
void buzzer_tone(uint32_t freq, uint32_t duration_ms){

    if (freq == 0) { Delay_Ms(duration_ms); return; }

    uint16_t half_period_us = (uint16_t)(1000000UL / ((uint32_t)freq * 2));
    uint32_t cycles         = ((uint32_t)freq * duration_ms) / 1000;

    for (uint32_t i = 0; i < cycles; i++) {
        io_buzzflasher(1);
        Delay_Us(half_period_us);
        io_buzzflasher(0);
        Delay_Us(half_period_us);
    }
}

void buzzer_sweep(uint16_t start_freq, uint16_t end_freq, uint16_t duration_ms){
    uint16_t step_ms = 5;
    uint16_t steps   = duration_ms / step_ms;
    if (steps == 0) steps = 1;

    for (uint16_t i = 0; i < steps; i++) {
        int32_t freq = (int32_t)start_freq +
                       ((int32_t)((int16_t)(end_freq - start_freq)) * (int32_t)i) / steps;

        uint16_t half_period_us = (uint16_t)(1000000UL / ((uint32_t)freq * 2));
        uint32_t cycles         = ((uint32_t)freq * step_ms) / 1000;

        for (uint32_t j = 0; j < cycles; j++) {
            io_buzzflasher(1);
            Delay_Us(half_period_us);
            io_buzzflasher(0);
            Delay_Us(half_period_us);
        }
    }

}

void buzzer_shoot(){
    buzzer_sweep(2500, 200, 120);
}

void buzzer_hit(){
    buzzer_sweep(800, 400, 100);
    Delay_Ms(30);
    buzzer_sweep(600, 150, 100);
    Delay_Ms(40);
    buzzer_sweep(350, 60, 160);

}

// flash an LED (you guessed it! shitty implementation using delay! Not that it matters here)
void flash(){
    buzzer_hit();
    Delay_Ms(30);
    io_buzzflasher(1);
    Delay_Ms(30);
    io_buzzflasher(0);
}


// ============== STORAGE OF DATA IN FLASH MEMORY ===============

// ADDRESS BOOK
// Last 4KB are reserved for the user data, starting from 

#define STORAGE_ADDRESS_ID 0x08003FC0
#define STORAGE_ADDRESS_PLAYER_NAME 0x08003F80
#define STORAGE_ADDRESS_HIT_SONG 0x08003F40


void storage_save_id(){
    FLASH_Unlock_Fast();
    FLASH_ErasePage_Fast(STORAGE_ADDRESS_ID);
    FLASH_ProgramHalfWord(STORAGE_ADDRESS_ID, player_id);
    FLASH_Lock_Fast();
}

void storage_save_name(){
    FLASH_Unlock_Fast();
    FLASH_ErasePage_Fast(STORAGE_ADDRESS_PLAYER_NAME);

    uint16_t two_letters;
    for(uint16_t i=0; i<16; i+=2){
        // combine two letters into a single 16bit half word (smallest allowed write-to-flash size)
        two_letters = player_name[i+1];
        two_letters <<= 8;
        two_letters |= player_name[i];
        FLASH_ProgramHalfWord(STORAGE_ADDRESS_PLAYER_NAME + i, two_letters);
    }
    FLASH_Lock_Fast();
}

void storage_save_hit_song(){
    FLASH_Unlock_Fast();
    FLASH_ErasePage_Fast(STORAGE_ADDRESS_HIT_SONG);

    uint16_t two_notes;
    for(uint16_t i=0; i<16; i+=2){
        // combine two letters into a single 16bit half word (smallest allowed write-to-flash size)
        two_notes = hit_song[i+1];
        two_notes <<= 8;
        two_notes |= hit_song[i];
        FLASH_ProgramHalfWord(STORAGE_ADDRESS_HIT_SONG + i, two_notes);
    }
    FLASH_Lock_Fast();
}

void storage_load(){
    // Load the ID:
    player_id = *((uint8_t *)(STORAGE_ADDRESS_ID));
    // sanity check:
     if(player_id == 0xFF){player_id = 0x00;}

    // Load the player name:
    for(uint16_t i=0; i<16; i+=2){
        player_name[i] = *((uint8_t *)(STORAGE_ADDRESS_PLAYER_NAME + i));
        player_name[i+1] = *((uint8_t *)(STORAGE_ADDRESS_PLAYER_NAME + i+1));
    }
    // sanity check
    if(player_name[0] == 0xFF){player_name[0] = 0x00;}

    // Load the "Hit Song":
    for(uint16_t i=0; i<16; i+=2){
        hit_song[i] = *((uint8_t *)(STORAGE_ADDRESS_HIT_SONG + i));
        hit_song[i+1] = *((uint8_t *)(STORAGE_ADDRESS_HIT_SONG + i+1));
    }

     // sanity check
    if(hit_song[0] == 0xFF){hit_song[0] = 0x00;}
}

// xxxxxxxxxxxxxxxx END OF HARDWARE ABSTRACTION xxxxxxxxxxxxxxxxxxx


// ==================== TOP LEVEL GAME LOGIC ==================


// IR COMMUNICATION 
char key[] = {'P', 'E', 'W', '!'};
void process_rx_data(uint16_t rx){
    static uint16_t key_pointer = 0;
    if(rx == key[key_pointer]){
        key_pointer++;
        if(key_pointer==4){
            flash();

            key_pointer = 0;
        }
    }else{
        key_pointer = 0;
    }
}

void send_test(){
    ir_send_preamble();
    for(int i=0; i<16; i++){
        if(player_name[i] == '\0'){break;}
        send_byte(player_name[i]);
    }
    // in case of buzzing after there is no need for delay
    //Delay_Ms(50);
}

void send_pew(){
    ir_send_preamble();
    send_byte('P');
    send_byte('E');
    send_byte('W');
    send_byte('!');
    ir_send_ending_pulse();
    Delay_Ms(50);
}

// =========================================================
// ========================== MAIN =========================
// =========================================================
int main(void){
    // Init stuff
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    init_hardware();

    // Load configuration and game state from flash (name, "hit song", etc)
    storage_load();
  
    // =========================================================
    // ======================= MAIN LOOP =======================
    // =========================================================
    while(1){
        
        // Check if trigger button pressed, and if so, SHOOT
        // if(io_trigger_pressed()){
        //     send_test();
        //     buzzer_shoot();
        //     send_pew();
        //     Delay_Ms(200);
        // }

        // // check if there is a new byte in the IR RECEIVER
        // if (decode_rx()){
        //     // debug print it on the debug uart
        //     // USART_SendData(USART1, rx_data);

        //     // pass the new byte into processing
        //     process_rx_data(rx_data);
        // }

        for(uint16_t i=0; i<16; i++){
            USART_SendData(USART1, player_name[i]);
            Delay_Us(80);
        }

        Delay_Ms(500);

    }
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
}

