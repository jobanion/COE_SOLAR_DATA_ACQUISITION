// https://kobra.io/#/e/-KCiSjjOpSkdvn3PAKpB

#include "pic24_all.h"
#include "pic24_ports.h"
#include "stdio.h"
#include "stdlib.h"
//#include "string.h"

#define Vout    3.30
#define CS_1    _RB8         // chip select for ADC 1 (DC voltage 1-4)
#define CS_2    _RB7         // chip select for ADC 2 (DC voltage 5-6, AC voltage)
#define CS_3    _RB6         // chip select for ADC 3 (DC current, AC current)
#define heartbeatLED   _RB15

#define DC_voltage_1 0
#define DC_voltage_2 1
#define DC_voltage_3 2
#define DC_voltage_4 3
#define DC_voltage_5 4
#define DC_voltage_6 5
#define AC_voltage   6
#define DC_current_1 7
#define DC_current_2 8 
#define DC_current_3 9
#define DC_current_4 10
#define DC_current_5 11
#define DC_current_6 12
#define AC_current   13


#define NUM_SAMPLES 75
#define bit_conversion  3.3/1024.0
/*
#define DC_voltage_R1   120000
#define DC_voltage_R2   12000
#define DC_current_R1   "NA"
#define DC_current_R2   "NA"
#define AC_voltage_R1   10000
#define AC_voltage_R2   1500
#define AC_current_R1   1000
#define AC_current_R2   2000
#define DC_current_sens 1/185       // A per mV
#define AC_current_sens 1/100       // A per mV
#define AC_voltage_sens 115/8.85    // transformer ratio
#define AC_voltage_offset   1.667
#define AC_current_offset   2.50*/


// {chip select, command byte, number of measurements, voltage offset, resistor ratio, }
const int ADC_param [14][3] ={  {1, 0x00, 10},        // DC_voltage_1 
                                {1, 0x20, 10},        // DC_voltage_2 
                                {1, 0x40, 200},        // DC_voltage_3 
                                {1, 0x60, 200},        // DC_voltage_4 
                                {2, 0x00, 200},        // DC_voltage_5 
                                {2, 0x20, 200},        // DC_voltage_6 
                                {2, 0x40, 20},        // AC_voltage
                                {3, 0x80, 20},        // DC_current_1 
                                {3, 0x90, 20},        // DC_current_2  
                                {3, 0xA0, 200},        // DC_current_3 
                                {3, 0xB0, 200},        // DC_current_4  
                                {3, 0xC0, 200},        // DC_current_5 
                                {3, 0xD0, 200},        // DC_current_6 
                                {3, 0xF0, 20}};       // AC_current 


// {resistor ratio*sensitivity, offset*resistor ratio*sensitivity}
const float conv_param [14][2] ={   {0, 11},             // DC voltage1 calc
                                    {0, 11},             // DC voltage2 calc
                                    {0, 11},             // DC voltage3 calc
                                    {0, 11},             // DC voltage4 calc
                                    {0, 11},             // DC voltage5 calc
                                    {0, 11},             // DC voltage6 calc
                                    {1.667, 99.623},     // AC voltage calc
                                    {2.5, 7.842},        // DC current1 calc 7.142  {2.4745, 7.842}
                                    {2.5, 0.00541},      // DC current2 calc
                                    {2.5, 0.00541},      // DC current3 calc
                                    {2.5, 0.00541},      // DC current4 calc
                                    {2.5, 0.00541},      // DC current5 calc
                                    {2.5, 0.00541},      // DC current6 calc
                                    {2.5, 0.0150}};      // AC current calc
                                    
                                   
void setupSPI_ADC()
{
    
    IFS0bits.SPI1IF = 0; // Clear the Interrupt flag
    IEC0bits.SPI1IE = 0; // Disable the interrupt

    // SPI1CON1 Register Settings
    SPI1CON1bits.DISSCK = 0; // Internal serial clock is enabled
    SPI1CON1bits.DISSDO = 0; // SDOx pin is controlled by the module
    SPI1CON1bits.MODE16 = 0; // Communication is word-wide (16 bits)
    SPI1CON1bits.MSTEN = 1; // Master mode enabled
    SPI1CON1bits.SMP = 0; // Input data is sampled at the middle of data output time
    SPI1CON1bits.CKE = 0; // Serial output data changes on transition from
    SPI1CON1bits.SPRE = SEC_PRESCAL_8_1;
    SPI1CON1bits.PPRE = PRI_PRESCAL_4_1;
    
    // Idle clock state to active clock state
    SPI1CON1bits.CKP = 0; // Idle state for clock is a low level;

    CONFIG_SDO1_TO_RP(9);   //use RP9 (Pin 18) for SDO (MOSI)
    CONFIG_RB9_AS_DIG_OUTPUT();   //Ensure that this is a digital pin
    
    CONFIG_SCK1OUT_TO_RP(5);   //use RP5 (Pin 14) for SCLK
    CONFIG_RB5_AS_DIG_OUTPUT();   //Ensure that this is a digital pin
  
    CONFIG_SDI1_TO_RP(4);      //use RP4 (Pin 11) for SDI (MISO)
    CONFIG_RB4_AS_DIG_INPUT();   //Ensure that this is a digital pin
    
    CONFIG_RB6_AS_DIG_OUTPUT();  // use RB6 (Pin 15) for ADC3 CS (chip select)
    CONFIG_RB7_AS_DIG_OUTPUT();  // use RB7 (Pin 16) for ADC2 CS (chip select)
    CONFIG_RB8_AS_DIG_OUTPUT();  // use RB8 (Pin 17) for ADC1 CS (chip select)
    
    // initializing chip selects high (ADC inactive)
    CS_1 = 1;    
    CS_2 = 1;
    CS_3 = 1;
    
    SPI1STATbits.SPIEN = 1;    //enable SPI mode
}

// function to write to then read from ADC
// write to ADC in 8-bit chunks
// read back in 8 bit chunks
uint8_t spiWriteRead (uint8_t toWrite)
{
    SPI1BUF = toWrite;              // read
    
    while (!SPI1STATbits.SPIRBF);   // <======= !!!!!LOOK HERE!!!!!
            
    return SPI1BUF;                 // write 
}

// int chip : chip # (for chip select)
// int spi_bytes : spi data to send
// char *save_data : spi data received buffer/register
// int start_val : start index for saving data (user makes sure it doesn't overflow when calling function)

int acquire_ADC_data(int meas_type, uint16_t *master_data, char * message)
{
    uint8_t digital_data [2][ADC_param[meas_type][2]];
    int counter = 0;
    int i = 0;
    uint16_t ADC_data = 0;
    uint8_t test_input1, test_input2;

    //printf("%s %d\n", message, meas_type);
    //printf("[%d %d %d]", ADC_param[meas_type][0], ADC_param[meas_type][1], ADC_param[meas_type][2]);
    
    while(counter < ADC_param[meas_type][2])
    {
        // set chip select low
        if (ADC_param[meas_type][0] == 1){
            CS_1=0;
            NOP();
            CS_1=0;
            NOP();
            //printf("::setting cs_1::");
        } else if (ADC_param[meas_type][0] == 2){
            CS_2=0;
            NOP();
            CS_2=0;
            NOP();
            /*if (CS_2 != 0){
                //#printf("Error, CS_2 didn't go high!");
                CS_2 = 0;
            }*/
        } else if (ADC_param[meas_type][0] == 3){
            CS_3=0;
            NOP();
            CS_3=0;
            NOP();
        }

        // send preamble bit
        spiWriteRead(0x01);        
        // ADC channel and mode 
        test_input1 = spiWriteRead(0x00); //ADC_param[meas_type][1]
        test_input2 = spiWriteRead((uint8_t)ADC_param[meas_type][1]);//ADC_param[meas_type][1]); //0x06
        digital_data[1][counter] = test_input1;//ADC_param[meas_type][2]);
        digital_data[0][counter] = test_input2;
        
        //ADC_data = (((digital_data[1][counter] << 8) | digital_data[0][counter]) & 0x03FF);
        //printf("RAW_ADC_DATA: %d", ADC_data);
        
        CS_1 = 1;
        NOP();
        CS_1 = 1;
        /*if (CS_1 != 1){
            //#printf("Error, CS_2 didn't go high!");
            CS_1 = 1;
        }*/
        CS_2 = 1;
        NOP();
        CS_2 = 1;
        if (CS_2 != 1){
            //#printf("Error, CS_2 didn't go high!");
            CS_2 = 1;
        }
        CS_3 = 1;
        NOP();
        CS_3 = 1;
        /*if (CS_3 != 1){
            //#printf("Error, CS_2 didn't go high!");
            CS_3 = 1;
        }*/
        counter++;
        NOP();
    }

    // analog input conversion
    
    while(i < ADC_param[meas_type][2])
    {
            // reconstruct 10-bit digital output 
        //ADC_data = (((digital_data[1][i] & 0x03) << 8) | digital_data[0][i]);
        ADC_data = (((digital_data[1][i] << 8) | digital_data[0][i]) & 0x03FF);
        printf("Test Data %d:0x%.2X: %d %f\n", ADC_param[meas_type][0], ADC_param[meas_type][1], ADC_data, (float)ADC_data*bit_conversion);
            // convert to analog value
        //master_data[i] = ((double)(ADC_data*bit_conversion - conv_param[meas_type][0]) * conv_param[meas_type][1]);
        master_data[i] = (uint16_t) ADC_data;
        //printf("master_data[%d] = %d\n", i, master_data[i]);
        
        i++;
    }

    //#printf("Conversion done\n");
    return ADC_param[meas_type][2];

}

// send ADC data to beaglebone
// format: [header][Voltage_data][Current_data]
// [header] = PX[#voltage_points][#current_points][size of measurement]
// P : 1 byte
// X : 1 byte, character (0, 1, 2, 3, 4, 5, or 6)
// [#voltage_points]: 2 bytes, integer, number of voltage points
// [#current_points]: 2 bytes, integer, number of current points
// [size of measurement]: 1 byte, integer, size of the points (bytes)
int send_ADC_data(int meas_type_voltage, int meas_type_current, uint16_t *master_data_voltage, uint16_t *master_data_current){
    int ix = 0;
    uint16_t voltage_headder_info = (uint16_t) ADC_param[meas_type_voltage][2];
    uint16_t current_headder_info = (uint16_t) ADC_param[meas_type_current][2];
    uint8_t measurement_size = 0x02;
    
    printf("Voltage:%d\n", meas_type_voltage);
    printf("Voltage_headder:%d\n", voltage_headder_info);
    printf("Current:%d\n", meas_type_current);
    printf("Current_headder:%d\n", current_headder_info);        
    
    //print out data (for stuff)
    for (ix = 0; ix < voltage_headder_info; ix++){
   //     printf("\nmaster_data_voltage[%d] = %d", ix, master_data_voltage[ix]);
    }
    for (ix = 0; ix < current_headder_info; ix++){
   //     printf("\nmaster_data_current[%d] = %d", ix, master_data_current[ix]);
    }
    
    
    // send header information
    while (U1STAbits.UTXBF == 1);
    U1TXREG = 'P';                              // send sync character
    while (U1STAbits.UTXBF == 1);
    U1TXREG = ((uint8_t) meas_type_voltage + '0');      // send measurement number
    
    while (U1STAbits.UTXBF == 1);
    U1TXREG = (voltage_headder_info & 0xff);    // send number of voltage datapoints
    while (U1STAbits.UTXBF == 1);
    U1TXREG = ((voltage_headder_info >> 8) & 0xff);
    
    while (U1STAbits.UTXBF == 1);
    U1TXREG = (current_headder_info & 0xff);    // send number of current datapoints
    while (U1STAbits.UTXBF == 1);
    U1TXREG = ((current_headder_info >> 8) & 0xff);
    
    while (U1STAbits.UTXBF == 1);
    U1TXREG = measurement_size;
    while (U1STAbits.UTXBF == 1);
    U1TXREG = 0x00;                              // send measurement size
    
    //while (U1STAbits.UTXBF == 1);
    //U1TXREG = ' ';
    //while (U1STAbits.UTXBF == 1);
    //U1TXREG = 'D';
    
    
    // calculate and send data
    // send voltage data
    for (ix = 0; ix < voltage_headder_info; ix++){
        while (U1STAbits.UTXBF == 1);
        U1TXREG = (master_data_voltage[ix] & 0xff);
        while (U1STAbits.UTXBF == 1);
        U1TXREG = ((master_data_voltage[ix] >> 8) & 0xff);
    }
    //while (U1STAbits.UTXBF == 1);
    //U1TXREG = 'U';                              // send sync character
    // send current data
    for (ix = 0; ix < current_headder_info; ix++){
        while (U1STAbits.UTXBF == 1);
        U1TXREG = (master_data_current[ix] & 0xff);
        while (U1STAbits.UTXBF == 1);
        U1TXREG = ((master_data_current[ix] >> 8) & 0xff);
    }
    // completed sending information, return
    return 1;
}

//float master_data1[2][20];/*[20];*/
//float master_data2[2][20];
int main (void) 
{
    configBasic(HELLO_MSG);
    setupSPI_ADC();

    
    //#printf("got here: HIIIII\n");
    heartbeatLED = 1;       // set heartbeat LED to ON state (turn LED on)
    CS_1 = 1;
    NOP();
    CS_1 = 1;
    CS_2 = 1;
    NOP();
    CS_2 = 1;
    if (CS_2 != 1){
        //printf("Error, CS_2 didn't go high!");
        CS_2 = 1;
    }
    CS_3 = 1;
    NOP();
    CS_3 = 1;
    
    char dc_voltage_message[] = "\nacquiring ADC data from DC voltage:";
    char FTDI_BUFF [300];
    char FTDI_BUFF2 [300];
    uint16_t master_data[2][NUM_SAMPLES];
    long int Voltage_data_sum = 0;
    long int Current_data_sum = 0;
    double Voltage_data_average = 0;
    double Current_data_average = 0;
    int num_samples_Voltage = 0;
    int num_samples_Current = 0;
    int i = 0;
    int ix = 0;
    
    //int values[] = {1,6};
    
    /*while(1){
        if (CS_1 == 0) {
            CS_1 = 1;
            CS_2 = 1;
            CS_2 = 1;
            CS_3 = 1;
            if (CS_1 != 1){
                printf("Error, CS_1 didn't go high!");
                CS_1 = 1;
            }
            if (CS_2 != 1){
                printf("Error, CS_2 didn't go high!");
                CS_2 = 1;
            }
            if (CS_3 != 1){
                printf("Error, CS_3 didn't go high!");
                CS_3 = 1;
            }
            
            printf("1HIHIHI\n");
        } else {
            CS_1 = 0;
            CS_2 = 0;
            CS_2 = 0;
            CS_3 = 0;
            printf("1LOWLOWLOW\n");
        }
        DELAY_MS(3000);
    }*/
    while (1) 
    {
        //if (ix == 0) ix = 1;
        //else ix = 0;
        
        num_samples_Voltage = acquire_ADC_data(ix, master_data[0], dc_voltage_message);
        num_samples_Current = acquire_ADC_data(ix+7, master_data[1], dc_voltage_message);
        
        Voltage_data_sum = 0;
        Current_data_sum = 0;
        
        // averaging data from measurements
        for(i = 0; i < num_samples_Voltage; i++)
        {
            //printf("a) %li = %i + %li ::: ", Voltage_data_sum + master_data[0][i], master_data[0][i], Voltage_data_sum);
            Voltage_data_sum = Voltage_data_sum + master_data[0][i];
            Current_data_sum = Current_data_sum + master_data[1][i];
            //printf("b)VOLTAGE_SUM[%d] = %li\n", i, Voltage_data_sum);
        }
        
        Voltage_data_average = (double) (Voltage_data_sum/((long double)NUM_SAMPLES));
        Current_data_average = (double) (Current_data_sum/((long double)NUM_SAMPLES));

        // (3) Write data to output port
        //char printString[] = "P1 %f %f (single: %f)\n";
        sprintf(FTDI_BUFF, "U%d %f %f (single: %f)\n", ix,(double)Voltage_data_average, (double)((double)(Voltage_data_average*bit_conversion - conv_param[0][0]) * conv_param[0][1]), (double)((double)(master_data[0][0]*bit_conversion - conv_param[0][0]) * conv_param[0][1]));//Voltage_data_average, Current_data_average);
        sprintf(FTDI_BUFF2, "U%d %f %f (single: %f)\n", ix+7,(double)Current_data_average, (double)((double)(Current_data_average*bit_conversion - conv_param[DC_current_1][0]) * conv_param[DC_current_1][1]) + 0.2, (double)master_data[1][0]*bit_conversion);
        //printf("%s", FTDI_BUFF);
        //printf("%s", FTDI_BUFF2);
        uint8_t PIE[] = {(uint8_t) 48, (uint8_t) 49}; // 0 1
        //int ix = 0;
        outString(FTDI_BUFF);
        outString(FTDI_BUFF2);
        send_ADC_data(ix, ix+7, master_data[0], master_data[1]);
        /*while (U1STAbits.UTXBF == 1);
        U1TXREG = PIE[0];       // 0
        for (ix = 0; ix < 2; ix++){
        while (U1STAbits.UTXBF == 1);
            U1TXREG = (PIE[ix]);        // 0 1
        }
        outString((char*)PIE);          // 0 1
        */
        doHeartbeat();
        DELAY_MS(1000);
    }
       
} 