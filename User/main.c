#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "lcd.h"
#include "w25qxx_spi_flash.h"
//#include "fontupd.h"
#include "text.h"
//#include "exfuns.h"
#include "malloc.h"
//#include "rtc.h"
#include "usart3.h"
#include "hc05.h"
#include "string.h"
#include "GBK_LibDrive.h"
#include "tim.h"
#include "key.h"

#define LINE_SIZE		 16

//接收的信息分为两部分存储 ，时间和待显示的文本
message_obj receive_BUF[10];



//显示ATK-HC05模块的连接状态
void HC05_Sta_Show(void)
{												 
	if(HC05_LED)LCD_ShowString(5,10,120,16,16,BLACK,(u8*)"STA:Connect     ");			//连接成功
	else LCD_ShowString(5,10,120,16,16,BLACK,(u8*)"STA:DisConnect");		 			//未连接				 
}	



int main(void)
{
	u8 reclen = 0; //接收数据长度
	u8 line = 62;//显示行数
	u8 receive_num = 0;//接收次数，首次/非首次
	u8 key = 0;
	u8 key_press = 0;
	u8 temp[50];
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//中断优先级2：2
	uart_init(115200);//串口1 初始化
	delay_init();
	LCD_Init();
	printf("LCD_ID:%X\r\n",lcddev.id);
	W25Qxx_Init();
	printf("FLAHS_ID:0X%X\r\n",W25Qxx_ReadID());
	GBK_Lib_Init();//硬件GBK字库初始化
	KEY_Init();
	TIM_Init(9999,7199);//10Khz的计数频率,  延时1s == 1000 000us 
	
	
	delay_ms(100);
	LCD_ShowString(30,70,200,16,16,RED,(u8*)"The System is Initing!"); 
//	my_mem_init(SRAMIN);		//初始化内部内存池
//	exfuns_init();				//
//	printf("内存申请函数返回值：%d\r\n",exfuns_init());//0,返回成功	
//	printf("实时时钟初始化，返回值：%d\r\n",RTC_Init());	//0-初始化成功
//	RTC_Init();

//	f_mount(fs[1], "1", 1);//挂载外部flash
	
//	while(font_init())//字库是否挂载成功
//	{
//		printf("字库挂载失败\r\n");
//		LCD_ShowString(30, 100, 200, 200, 24, (u8*)"Font mounting error");//挂载失败
//	}
	
	
	while(HC05_Init()) 		//初始化ATK-HC05模块  
	{
		LCD_ShowString(30,90,200,16,16,RED,(u8*)"ATK-HC05 Error!"); 
		delay_ms(200);
		LCD_ShowString(30,90,200,16,16,RED,(u8*)"Please Check!!!"); 
		delay_ms(200);
	}
	
	LCD_Clear(WHITE);
	GBK_Show_Str(50,30, 200,300,(u8*)"格林贝演讲辅助系统",16, RED, WHITE, 0);
	GBK_Show_Str(5, 46, 200,300,(u8*)"接收数据为：",16, RED, WHITE, 0);
	memset(USART3_RX_BUF, 0, sizeof(USART3_RX_BUF));//清空数组内元素，使其为0
	
	
	while(1)
	{
		HC05_Sta_Show();//显示连接状态

		if(USART3_RX_STA & 0x8000)	//接收到一次数据
		{	
			reclen = USART3_RX_STA&0X7FFF;	//得到数据长度
			USART3_RX_BUF[reclen] = 0;//添加结束符
			USART3_RX_STA = 0;//清除接收标志位
			
			
			mesg_Analysis(receive_BUF,USART3_RX_BUF);
//			sprintf((char*)temp,"%d\n",receive_BUF[0].time);
//			puts((char*)temp);
			printf("time:%d ,text:%s\n",receive_BUF[0].time,receive_BUF[0].text);
			
			if(receive_num == 0)//第一次接收
			{
				receive_num++;
			}
			else//再次接收
			{
				line += LINE_SIZE;//显示位置下移
				receive_num++;
			}
			printf("receive: %s,strlen： %d\n",USART3_RX_BUF,strlen((const char*)USART3_RX_BUF));
		}
	
		
		
		GBK_Show_Str(8,line, 500,500,  USART3_RX_BUF,16, RED, WHITE, 0);
		
		if(receive_num >13)	
		{
			LCD_Clear(WHITE);
			GBK_Show_Str(50,30, 200,300,"格林贝演讲辅助系统",16, RED, WHITE, 0);
			GBK_Show_Str(5, 46, 200,300,"接收数据为：",16, RED, WHITE, 0);
			receive_num = 0;
			line = 62;
		}
		//memset(USART3_RX_BUF, 0, sizeof(USART3_RX_BUF));//清空数组内元素，使其为0		

		//按键按下
		key = KEY_Scan(1);
		if(key == KEY0_PRES)
		{
			if(key_press == 0)	//第一次按下，开始计时
			{
				key_press = 1;//按键状态反转
				TIM_Set(TIM4,ENABLE);//开定时器
			}
			else	//暂停计时
			{
				key_press = 0;//按键状态反转
				TIM_Set(TIM4,DISABLE);//关闭定时器
				
			}
		} 

		if(Is_TimeOut(30))	//判断是否已经到达设定时间
		{
			TIM_Set(TIM4,DISABLE);
		    printf(" time out !!! nowtime is %d\n",counter.sec);
			counter.hour = counter.min = counter.sec = 0;
		
		}			
		
		LCD_ShowxNum(80,220,counter.min,2,16,RED,0);//显示分钟
		LCD_ShowxNum(100,220,counter.sec,2,16,RED,0);//显示秒

		
	}//while(1)
	
}



