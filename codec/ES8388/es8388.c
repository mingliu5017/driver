/*----------------------------------------------
//Initialize CODEC after power up
// MIC TO LIN1&RIN1, DIFFERENTIAL MODE
// LOUT1&ROUT1 TO HEADPHONE
----------------------------------------------*/
void ES8388_INI(void)
{
  unsigned char i;
  WriteReg(0x01,0x58);
  WriteReg(0x01,0x50);
  WriteReg(0x02,0xF3);
  WriteReg(0x02,0xF0);
  WriteReg(0x2B,0x80);
  WriteReg(0x00,0x06);
  WriteReg(0x08,0x00);    // codec in i2s master mode
  WriteReg(0x04,0x00);
  WriteReg(0x19,0x02);
  WriteReg(0x09,0x88);		// left pga =+24db, right pga = +24db
  WriteReg(0x0A,0x00);    //lin1 rin1 for adc, stereo mode
  WriteReg(0x0B,0x02);
  WriteReg(0x0C,0x0C);    //adc i2s-16bit
  WriteReg(0x0D,0x02);    
  WriteReg(0x10,0x00);    //ladc volume = 0db
  WriteReg(0x11,0x00);    //radc volume = 0db
  WriteReg(0x17,0x18);		//dac i2s-16bit
  WriteReg(0x18,0x02);
  WriteReg(0x1A,0x00);    //ldac volume =0db
  WriteReg(0x1B,0x00);    //rdac volume = 0db
  WriteReg(0x26,0x09);    //lin2 rin2 for bypass in
  WriteReg(0x27,0xd0);    //ldac+lin2 to lmixer, if only use lin2 to lmixer, reg0x27 = 0x50
  WriteReg(0x2A,0xd0); 		//rdac+lin2 to rmixer, if only use rin2 to rmixer, reg0x2a = 0x50
  WriteReg(0x03,0x09);   // power up adc
  WriteReg(0x02,0x00);   // start stae machine
  DelayMs(500);
  WriteReg(0x04,0x3c);   // power up dac and line out
  for(i=0;i<0x1B;i++)		// set line output volume
  {
   WriteReg(0x2E,i);
   WriteReg(0x2F,i);
   WriteReg(0x30,i);
   WriteReg(0x31,i);
  }
}
/*----------------------------------------------
//SET CODEC INTO STANDBY MODE for min. power consumption
----------------------------------------------*/
void ES8388_STANDBY(void)
{
  unsigned char i;
  WriteReg(0x19, 0x06);    //mute dac
  for(i=0;i<0x1B;i++)
  {
   WriteReg(0x2E, 0x1A-i);
   WriteReg(0x2F, 0x1A-i);
   WriteReg(0x30, 0x1A-i);
   WriteReg(0x31, 0x1A-i);
  }
  WriteReg(0x04, 0x00);
  WriteReg(0x04, 0xC0);  //power down dac and line out
  WriteReg(0x03, 0xFF);  //power down adc and line in
  WriteReg(0x02, 0xF3);  //stop state machine
  WriteReg(0x2B, 0x9C);  //disable mclk
  WriteReg(0x00, 0x00);
  WriteReg(0x01, 0x58);
}
/*----------------------------------------------
//Resume Codec from standby mode
----------------------------------------------*/
void ES8388_RESUME(void)
{  
  unsigned char i;
  WriteReg(0x2B, 0x80);   //enable mclk
  WriteReg(0x01, 0x50);
  WriteReg(0x00, 0x36);
  WriteReg(0x02, 0x00);   //start state machine
  WriteReg(0x03, 0x09);   //power up adc and line in
  DelayMs(100);
  WriteReg(0x04, 0x00);
  WriteReg(0x04, 0x3c);   //power up dac and line out
  for(i=0;i<0x1B;i++)     //set lineout volume
  {
   WriteReg(0x2E, i);
   WriteReg(0x2F, i);
   WriteReg(0x30,i);
   WriteReg(0x31,i);
  }
  WriteReg(0x19, 0x02);   //ummute DAC
}
