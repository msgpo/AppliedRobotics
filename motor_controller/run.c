



#include <stdio.h>
#include <math.h>
#include "kernel.h"
#include "ecrobot_interface.h"
DeclareTask(task1);
DeclareTask(task2);
DeclareCounter(SysTimerCnt);
DeclareCounter(SysTimerCnt2);
int prev_rev_A = 0;
int prev_rev_B = 0;
double L = 	130; //space between wheels in mm
double R = 28; //radius wheel in mm
double gain = 10;
double alpha = 0.075;
double T = 0.005; // period sampling time
double dis1=100; 
double dis2=150; 
double dis3=300; 
double vel1 = 2.0;
double vel2 = 5.0;
double vel3 = 7.0;
double reference_speed = get_reference_speed(2*M_PI * (current_rev_A + current_rev_B) / 360) * R); // calculate distance in real time 
int canModify = 0;



void user_1ms_isr_type2(void)
{
  StatusType ercd, ercd2;
  ercd = SignalCounter(SysTimerCnt);
  ercd2 = SignalCounter(SysTimerCnt2);
  if (ercd != E_OK || ercd2 != E_OK){ShutdownOS(ercd);}
}
double speed_A(double raw_speed) {
	static double prev_speed = 0.0;
	prev_speed = alpha * raw_speed + (1-alpha)*prev_speed;
	return prev_speed;
}
double speed_B(double raw_speed) {
	static double prev_speed = 0.0;
	prev_speed = alpha * raw_speed + (1-alpha)*prev_speed;
	return prev_speed;
}
double controller_A(double u_2) {
	static double u_0 = 0.0, u_1 = 0.0;
	static double y_0 = 0.0, y_1 = 0.0;

	double y_2 = 1/(4 + 42 * T) *
		((gain * (u_2 * (4 + 100*T*T + 40*T) + u_1 * (-8 + 200*T*T) + u_0 * (4 - 40*T + 100*T*T))) - y_1 * (-8) - y_0 * (4 - 42*T));
	u_0 = u_1;
	u_1 = u_2;
	y_0 = y_1;
	y_1 = y_2;
	return y_2;
}
double controller_B(double u_2) {
	static double u_0 = 0.0, u_1 = 0.0;
	static double y_0 = 0.0, y_1 = 0.0;

	double y_2 = 1/(4 + 42 * T) *
		((gain * (u_2 * (4 + 100*T*T + 40*T) + u_1 * (-8 + 200*T*T) + u_0 * (4 - 40*T + 100*T*T))) - y_1 * (-8) - y_0 * (4 - 42*T));
	u_0 = u_1;
	u_1 = u_2;
	y_0 = y_1;
	y_1 = y_2;
	return y_2;
}

double get_reference_speed(double distance) {
   if (distance < dis1){
        
        return vel1;  
   }
   if (distance < dis2){
         
         return vel2; 
   }
   if (distance < dis3){

         return vel3;
   }  
   return 0;
}





TASK(task1) // called every 5 ms
{
	//get motor evolution
	int current_rev_A = nxt_motor_get_count(NXT_PORT_A);
	int current_rev_B = nxt_motor_get_count(NXT_PORT_B);
	//find out the speed
	int space_A = current_rev_A - prev_rev_A; //how much space it takes in degree motor A
	int space_B = current_rev_B - prev_rev_B; //how much space it takes in degree motor A
	//speed_deg = space / time_step; // degree / s
	double w_A = speed_A((space_A * (M_PI / 180.0)) / T); // radiant / s
	double w_B = speed_B((space_B * (M_PI / 180.0)) / T); // radiant / s
	double velocity = (w_A + w_B) / 2 * R; // speed of the vehicle in radiant / s
	double error_w = (w_B - w_A) / L * R; // it has to be = 0

	double error_A = reference_speed - w_A;
	double error_B = reference_speed - w_B;

	// in milliseconds: 1 wait -> 5ms, 2 wait -> 10ms, ...
	// our settling time is 0.4ms and our sampling time is 0.5ms,
	// so it is useless.. it was written to have a complete code
	int wait = 1; //1 is fine for us
	if(canModify == wait){
		//ecrobot_sound_tone(1500, 2, 50); //1500KHz, 2ms, volume:50
		double err = w_B - w_A;
		error_A += err / 2;
		error_B += -err / 2;
	}
	canModify = 0;

	double power_A = controller_A(error_A);
	double power_B = controller_B(error_B);

	prev_rev_A = current_rev_A;
	prev_rev_B = current_rev_B;

	//adjust the motor power
	nxt_motor_set_speed(NXT_PORT_A, power_A, 1);
	nxt_motor_set_speed(NXT_PORT_B, power_B, 1);

	//print to display
    display_clear(0);

    display_goto_xy(0,0); //(horizontal [0:15], vertical[0:7])
    display_string("pwr_A:");
    display_goto_xy(7,0);
    display_int(power_A, 3);

    display_goto_xy(0,1);
    display_string("err_A:");
    display_goto_xy(7,1);
    display_int(error_A*100, 3);

    display_goto_xy(0,2);
	display_string("pwr_B:");
	display_goto_xy(7,2);
	display_int(power_B, 3);

	display_goto_xy(0,3);
	display_string("err_B:");
	display_goto_xy(7,3);
	display_int(error_B*100, 3);

	display_goto_xy(0,4);
	display_string("err_w:");
	display_goto_xy(7,4);
	display_int(error_w*100, 3);

    display_update();
    TerminateTask();
}
int i = 0;
TASK(task2) // called every 5 ms
{
	canModify++;
	TerminateTask();
}