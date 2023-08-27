#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <dpp/dpp.h>

#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>

using namespace cv;
using namespace std;


bool bot_connected = false;

map<string, string> config {
    {"discord_bot_token", ""},
    { "username", "" },
    { "password", "" },
    { "ip", "" },
    { "port", "" },
    { "channel", "1" },
    { "subtype", "1" },
    { "name", "" },
    { "motion", "false" },
    { "people", "false" },
    { "discord_channel", "" },
    { "discord_server", "" },
    { "headless", "false" },
};


void read_config(string fname)
{
    fstream config_file;
    config_file.open(fname, ios::in);

    if (config_file.is_open())
    {
        string line;
        
        while (getline(config_file, line))
        {
            int d = line.find("=");
            string key = line.substr(0, d);
            string val = line.substr(d+1, line.length());
            if (config.find(key) != config.end())
            {
                config[key] = val;
            }
        }

        config_file.close();
    }
}


int main() 
{
    // Read config file
    read_config("config");

    ostringstream oss;
    if (config["port"].compare("") != 0)
    {
        oss << "rtsp://" << config["username"] << ":" << config["password"] << "@" << config["ip"] << ":" << config["port"] << "/cam/realmonitor?channel=" << config["channel"] << "&subtype=" << config["subtype"];
    }
    else
    {
        oss << "rtsp://" << config["username"] << ":" << config["password"] << "@" << config["ip"] << "/cam/realmonitor?channel=" << config["channel"] << "&subtype=" << config["subtype"];
    }
    string url = oss.str();

    VideoCapture cap(url, CAP_GSTREAMER);

    HOGDescriptor hog;
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
    
    while (!cap.isOpened()) {
        
        cerr << "Error opening video stream." << endl;
        this_thread::sleep_for(chrono::milliseconds(2000));
        cap.open(url);
        //return 1;
    }

    // Create a window to display the video
    if (config["headless"].compare("false") == 0)
    {
        namedWindow("Camera Stream", cv::WINDOW_NORMAL);
    }

    dpp::cluster bot(config["discord_bot_token"]);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t& event) {
        bot_connected = true;
        });

    bot.start();

    dpp::message logon;
    ostringstream logon_message_s;
    logon_message_s << config["name"] << " logging on";
    string logon_message = logon_message_s.str();
    logon.set_content(logon_message);
    logon.channel_id = config["discord_channel"];
    bot.message_create(logon);

    while (true) {
        Mat frame;
        cap >> frame; // Capture frame from RTSP stream

        vector<cv::Rect> detected_people;
        hog.detectMultiScale(frame, detected_people, 1.05, cv::Size(8, 8));

        // Draw rectangles around detected people
        for (const auto& rect : detected_people) {
            rectangle(frame, rect, Scalar(0, 0, 255), 2); // Red color (BGR format)
        }

        if (frame.empty()) {
            cerr << "End of video stream." << endl;
            break;
        }

        if (config["headless"].compare("false") == 0)
        {
            imshow("Camera Stream", frame);
        }

        if (detected_people.size() > 0 && bot_connected)
        {
            imwrite("person.jpg", frame);
            dpp::message m;
            m.set_content("Person detected!");
            m.channel_id = config["discord_channel"];
            m.add_file("person.jpg", dpp::utility::read_file("person.jpg"));
            bot.message_create(m);
        }

        // Press 'q' to exit the loop
        if (waitKey(1) == 'q') {
            break;
        }
    }

    dpp::message logoff;
    ostringstream logoff_message_s;
    logoff_message_s << config["name"] << " logging off";
    string logoff_message = logoff_message_s.str();
    logoff.set_content(logon_message);
    logoff.channel_id = config["discord_channel"];
    bot.message_create(logoff);

    cap.release();
    cv::destroyAllWindows();

    return 0;
}
