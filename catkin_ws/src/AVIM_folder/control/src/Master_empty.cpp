#include<iostream>
#include<ros/ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Int16.h>
#include <std_msgs/UInt8.h>
#include<math.h>
#include<string>
#include <vector>


int LANE_DRIVING = 0;

static std::map<int, std::string>task_names{
     { LANE_DRIVING,"Lane driving"}
};


class Task{
    public:
        std::string name;
        int ID;

        Task(int task_identifier){
            this->ID = task_identifier;
            this->name = task_names[this->ID];
        }
};


class Master{
    private:
        ros::NodeHandle nh_;
        ros::Subscriber distance_center;
        ros::Publisher angle_pub;
        ros::Publisher speed_pub;

        std_msgs::Int16 angle_message;
        std_msgs::Int16 speed_message;

        int dist_now;
        int angle_now;
        int angle_last;
        float kp_angle;
        float kd_angle;
        float error_angle;
        int u_angle;
        int angle_pd;
        int speed_pid;
        float kp_speed;
        int u_speed;
        std::vector<Task> task_pile; 
   
    public:
        //Constructor
        Master(Task task){
            //ros::Subscriber angle_now = nh.subscribe("/angle_line_now",1,angle_nowCallback);
            distance_center = nh_.subscribe("/distance_center_line",1, &Master::dist_center_clbk, this);
            angle_pub = nh_.advertise<std_msgs::Int16>("/AutoModelMini/manual_control/steering",1000);
            speed_pub = nh_.advertise<std_msgs::Int16>("/AutoModelMini/manual_control/speed",1000);
            dist_now = 0;
            angle_now = 0;
            angle_last = 0;
            kp_angle = 0.825;
            kd_angle = 0.0297;
            error_angle = 0.0;
            u_angle = 0;
            speed_pid = 0;
            kp_speed = 1.2645;
            u_speed = 0;
            this->add_task(task);
        }

        void dist_center_clbk(const std_msgs::Int16& dis_now_center){
            dist_now = static_cast<int>(dis_now_center.data);
            this->run();
            this->publish_policies();
            //ROS_INFO("Dist center %d", dist_now);
        }


        void add_task(Task task){
            this->task_pile.push_back(task);
        }

        void remove_task(){
            if (this->task_pile.size() > 1)
                this->task_pile.pop_back();
        }
        Task get_current_task(void){
            return this->task_pile.back();
        }

        void task_assigner(void){
            // Get the current task   
            Task current_task = get_current_task();
        }
        
        void task_solver(void){
            Task current_task = this->get_current_task();
            ROS_INFO_STREAM("[Current task]: " <<current_task.name);
            if (current_task.ID == LANE_DRIVING){
                on_lane();
            }
        }

        void run(void){
            this->task_assigner();
            this->task_solver();
        }

        void on_lane(void){
            u_angle = static_cast<int>(kp_angle*static_cast<float>(dist_now) + kd_angle*static_cast<float>(dist_now - angle_last));
            angle_pd = 90 + u_angle;
            if(angle_pd<= 45)
	            angle_pd = 45;
            else if(angle_pd >= 135)
	            angle_pd = 135;
            u_speed = static_cast<int>(kp_speed * dist_now);
            speed_pid = - 635 + abs(u_speed);
            if(speed_pid < - 635)
	            speed_pid = - 635;
            else if(speed_pid > 0)
	            speed_pid = 0;
            angle_last = angle_pd;
        }

        void publish_policies(){
            angle_message.data = angle_pd;
            angle_pub.publish(angle_message);
            speed_message.data = speed_pid;
            speed_pub.publish(speed_message);
        }
};


int main(int argc, char **argv){
    ros::init(argc, argv, "Master");
    Master *control = new  Master(Task(LANE_DRIVING));
    ros::spin();
    return 0;
}
