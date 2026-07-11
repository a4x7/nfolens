#include <ftxui/ftxui.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <atomic>

struct system_info{
    std::string hostname;
    std::string uptime;
    std::string cpu_freq;
    std::string memTotal;
    std::string memUsed;
    float memRatio;
    float cpuRatio;
    std::string received;
    std::string transmitted;
    std::string received_rate;
    std::string transmitted_rate;
};
system_info globals;

ftxui::App app = ftxui::App::FitComponent();
ftxui::Event changed = ftxui::Event::Special("changed");

std::mutex m1;
std::atomic<bool> quit = false;

void ftxui_main(){
    using namespace ftxui;
    Component elmnt = Renderer([]{
        using namespace std::literals::chrono_literals;
        system_info locals; 
        {
            std::lock_guard<std::mutex> guard(m1);
            locals = globals;
        }
        std::string str_memRatio = std::to_string(locals.memRatio*100);
        std::string str_cpuRatio = std::to_string(locals.cpuRatio*100);
        Element header = text("System Information") | bold | hcenter | bgcolor(Color::CornflowerBlue) | color(Color::Black);
        Table t({
            {
                text("Hostname") | color(Color::CornflowerBlue),
                vbox({
                    text(globals.hostname),
                }),
            },
            {
                text("CPU Usage") | color(Color::CornflowerBlue),
                vbox({
                    text(locals.cpu_freq.substr(0, locals.cpu_freq.find(".")+4) + " GHz"),
                    hbox({
                    gauge(locals.cpuRatio) | color(LinearGradient()
                        .Angle(0)
                        .Stop(Color::Green1, 0)
                        .Stop(Color::Yellow1, 0.7)
                        .Stop(Color::Red1, 0.85)
                    ) | bgcolor(Color::Black) | borderStyled(Color::White),
                    text(str_cpuRatio.substr(0, str_cpuRatio.find(".")) + "%") | vcenter,
                    }),
                }),
            },
            {
                text("Used Mem") | color(Color::CornflowerBlue),
                vbox({
                    text(locals.memUsed.substr(0, locals.memUsed.find(".")+4) + " GB / " + locals.memTotal.substr(0, locals.memTotal.find(".")+4) + " GB"),
                    hbox({
                    gauge(locals.memRatio) | color(LinearGradient()
                        .Angle(0)
                        .Stop(Color::Green1, 0)
                        .Stop(Color::Yellow1, 0.7)
                        .Stop(Color::Red1, 0.85)
                    ) | bgcolor(Color::Black) | borderStyled(Color::White),
                    text(str_memRatio.substr(0, str_memRatio.find(".")) + "%") | vcenter,
                    }),
                }),
            },
            {
                text("Network Speed") | color(Color::CornflowerBlue),
                vbox({text("Download: " + locals.received_rate + "KB/s"), text("Upload: " + locals.transmitted_rate + "KB/s")})
            },
            {text("Uptime") | color(Color::CornflowerBlue), text(locals.uptime)},
        });
        t.SelectColumns(0,1).Separator();
        t.SelectColumn(1).DecorateCells(size(WIDTH, GREATER_THAN, 30));
        return vbox({
            header,
            separator(),
            t.Render(),
        }) | border;
        
    });
    Component component = CatchEvent(elmnt, [](Event event){
        if(event == Event::Character('q')){
            quit = true;
            app.ExitLoopClosure()();
            return true;
        }
        if(event == changed){
            return true;
        }
        return false; 
    });
    app.Loop(component);
    std::cout << "Exitted render loop\n";
}

void helper1(uint64_t factor){
    std::fstream fin1;
    std::fstream fin2;
    std::fstream fin3;
    std::fstream fin4;
    std::fstream fin5;
    std::fstream fin6;
    using namespace std::literals::chrono_literals;
    uint64_t prev_received = 0;
    uint64_t prev_transmitted = 0;
    uint64_t prev_cpuTotalTime = 0;
    uint64_t prev_cpuIdleTime = 0;
    {
        std::string hostname;
        fin6.open("/etc/hostname", std::ios::in);
        if(fin6.is_open()){
            fin6 >> hostname;
            fin6.close();
            {
                std::lock_guard<std::mutex> guard(m1);
                globals.hostname = hostname;
            }
            app.PostEvent(changed);
        } else {
            app.Exit();
            return;
        }
    }
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(factor));
        fin1.open("/proc/uptime", std::ios::in);
        fin2.open("/proc/cpuinfo", std::ios::in);
        fin3.open("/proc/meminfo", std::ios::in);
        fin4.open("/proc/net/dev", std::ios::in);
        fin5.open("/proc/stat", std::ios::in);
        if(fin1.is_open() && fin2.is_open() && fin3.is_open() && fin4.is_open() && fin5.is_open()){
            uint64_t int_uptime;
            std::string line_cpu_freq;
            double d_memTotal;
            double d_memAvailable;
            uint64_t int_received = 0;
            uint64_t int_transmitted = 0;
            uint64_t int_received_rate;
            uint64_t int_transmitted_rate;
            uint64_t int_cputTotalTime = 0;
            uint64_t int_cpuIdleTime;
            uint64_t int_cpuTotalRate;
            uint64_t int_cpuIdleRate;
            std::string buf;

            fin1 >> int_uptime;
            for(int i = 0; i < 8; i++)
                std::getline(fin2, line_cpu_freq);
            fin3 >> buf >> d_memTotal >> buf >> buf >> buf >> buf >> buf >> d_memAvailable;
            std::getline(fin4, buf); 
            std::getline(fin4, buf);
            while(fin4 >> buf)
                if(buf.find("wl") != std::string::npos || buf.find("en") != std::string::npos || buf.find("eth") != std::string::npos){
                    uint64_t temp;
                    fin4 >> temp;
                    int_received += temp;
                    for(int i = 0; i < 7; i++)
                        fin4 >> buf;
                    fin4 >> temp;
                    int_transmitted += temp;
                }
            fin5 >> buf;
            buf = "";
            char c;
            do {
                fin5.get(c);
            } while(c == ' ');
            int i = 0;
            uint64_t intBuf;
            bool q = false;
            do {
                switch(c) {
                    case ' ':
                        intBuf = std::stoi(buf);
                        buf = "";
                        int_cputTotalTime += intBuf;
                        i++;
                        if(i == 4)
                            int_cpuIdleTime = intBuf;
                        break;
                    case '\n':
                        intBuf = std::stoi(buf);
                        buf = "";
                        int_cputTotalTime += intBuf;
                        i++;
                        if(i == 4)
                            int_cpuIdleTime = intBuf;
                        q = true;
                        break;
                    default:
                        buf += c;
                }
                if(q)
                    break;
            } while(fin5.get(c));

            int_received_rate = (int_received - prev_received) * 1000 / factor;
            int_transmitted_rate = (int_transmitted - prev_transmitted) * 1000 / factor;
            int_cpuTotalRate = int_cputTotalTime - prev_cpuTotalTime;
            int_cpuIdleRate = int_cpuIdleTime - prev_cpuIdleTime;
            prev_cpuTotalTime = int_cputTotalTime;
            prev_cpuIdleTime = int_cpuIdleTime;
            prev_received = int_received;
            prev_transmitted = int_transmitted;

            fin1.close();
            fin2.close();
            fin3.close();
            fin4.close();
            fin5.close();

            std::string time;
            int hours = int_uptime / 3600;
            int mins = (int_uptime - 3600*hours) / 60;
            int secs = (int_uptime - 60*mins - 3600*hours);

            int start = line_cpu_freq.find(":")+2;
            line_cpu_freq = line_cpu_freq.substr(start);

            d_memTotal /= 1024*1024;
            d_memAvailable /= 1024*1024;

            std::lock_guard<std::mutex> guard(m1);
            using std::to_string;
            globals.uptime = to_string(hours) + "hrs " + to_string(mins) + "mins " + to_string(secs) + "s";
            globals.cpu_freq = std::to_string(std::stod(line_cpu_freq) / 1000);
            globals.memTotal = std::to_string(d_memTotal);
            globals.memUsed = std::to_string(d_memTotal-d_memAvailable);
            globals.memRatio = (d_memTotal - d_memAvailable) / d_memTotal;
            globals.received = std::to_string(int_received/(1024));
            globals.transmitted = std::to_string(int_transmitted/(1024));
            globals.received_rate = std::to_string(int_received_rate/(1024));
            globals.transmitted_rate = std::to_string(int_transmitted_rate/(1024));
            globals.cpuRatio = 1 - static_cast<double>(int_cpuIdleRate) / int_cpuTotalRate;

            app.PostEvent(changed);
        } else{
            app.Exit();
            std::fstream* arr[] = {&fin1, &fin2, &fin3, &fin4, &fin5};
            for(int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
                    if((*arr[i]).is_open())
                        (*arr[i]).close();
            break; 
        }
        if(quit)
            break;
    }
    std::cout << "Helper thread terminated\n";
}

int main(int argc, char** argv){

    uint64_t arg; 
    try{
        arg = std::stoi(argv[1]);
    } catch(std::logic_error){
        arg = 1000;
    }
    std::thread worker1(helper1, arg);

    ftxui_main();

    worker1.join();
    if(!quit){
        std::cerr << "Program terminated with exit code 1\n";
        return 1;
    }
    std::cout << "Program terminated with exit code 0\n";
    return 0;

}
