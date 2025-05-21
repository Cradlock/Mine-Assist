#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <any>
#include <filesystem>
#include <functional>
#include <SQLiteCpp/SQLiteCpp.h>
#include <cstdlib>
#include <windows.h>


using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;


string getExecutablePath(){
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return string(buffer);
}
fs::path exePath = fs::path(getExecutablePath()).parent_path();
const string PATHDB = (exePath / "db.sql").string();
const string PATHCONF = (exePath / "conf.json").string();
fs::path root  = fs::current_path();
SQLite::Database db(PATHDB,SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

using blankF = function<void()>;
using intF = function<void(int)>;
using strF = function<void(string)>;
using twoStrF = function<void(string,string)>;


struct command{
        string name,desc;
        any action;

        command(string n,string desc,any ac){
            this->name = n;
            this->desc = desc;
            this->action = ac;
        }
};


void getCurrentPath(){
   cout << "Programm running in -> " << endl;
   cout << fs::current_path().string() << endl;
}
const command pwd = command("pwd","get current path programm",blankF(getCurrentPath));



void RunTlanch(int id){
    ifstream file(PATHCONF);
    json j;
    file >> j;
    fs::path tlan_path = fs::path(j["minecraft"]); 
    file.close();

    if(id == -1){
        fs::path comm = tlan_path / "Tlauncher.exe";
        system(comm.string().c_str());
    }else{
        SQLite::Statement query(db,"SELECT title,has_mods,has_map,has_config FROM Builds WHERE id = ?");
        query.bind(1,id);
        if(query.executeStep()){
            string title = query.getColumn(0);
            fs::path path = exePath;
            
            boolean has_mods = query.getColumn(1);
            boolean has_map = query.getColumn(2);
            boolean has_config = query.getColumn(3);


            if(has_mods){

            }
            if(has_map){

            }
            if(has_config){

            }
        }else{
            cout << "Not Found this builds in db" << endl;
        }
    }
}
const command run_mine = command("run","Start tlaucnher with latest versions and mode: run <id build>",intF(RunTlanch));
const string add_mine = "add";
const string list_mine = "list";
const string update_mine = "update";

void RunVsCode(int selected_project){
    if(selected_project == -1){
        system("code");
    }else{
        SQLite::Statement query(db, "SELECT path FROM Projects WHERE id = ?");
        query.bind(1,selected_project);

        if(query.executeStep()){
            string pathP = query.getColumn(0).getString();
            string commandVs = "code " + pathP;
            system( commandVs.c_str() );
            
            string commandExp = "explorer " + pathP;
            system(commandExp.c_str());
        }else{   
            cout << "Not Found this project" << endl;
        }

    }
}
const command run_project = command("run-project","Run selected or latest project in programm vsode: run-project <id project>",intF(RunVsCode));

void InitProject(string name){
   SQLite::Statement insert(db,"INSERT INTO Projects (title,path) VALUES (?,?)");

   insert.bind(1,name);
   insert.bind(2,root.string());
   cout << "Initialize new project - " << name << " -p " << root.string() << endl;
   insert.exec();
}
const command init_project = command("init-project","initialize new project",strF(InitProject));

void ListProject(){
   SQLite::Statement query(db,"SELECT id,title,path FROM Projects");

   while(query.executeStep()){
     int id = query.getColumn(0);
     string title = query.getColumn(1);
     fs::path pathp = fs::path(query.getColumn(2).getString()); 

     cout << "(" << id << ")" << "\n   name: " << title << "\n   path: " << pathp.string() << endl;
   }


}
const command list_project = command("list-project","Get all project in your pc",blankF(ListProject));

void ClearDB(){
   db.exec("DELETE FROM Projects");
   cout << "DB cleared" << endl;
}
const command clear_project = command("clear-project","Clear Projects in DB",blankF(ClearDB));

void CreateProject(string name,string folder){
    SQLite::Statement insert(db,"INSERT INTO Projects (title,path) VALUES (?,?)");

    fs::path dir;
    if (folder == ".") {
        dir = root;
    } else {
        dir = root / folder;
    }
    
    

    string vscomm = "code " + dir.string(); 
    system(vscomm.c_str());

    string commandExp = "explorer " + dir.string();
    system(commandExp.c_str());

    insert.bind(1, name);
    insert.bind(2, dir.string());
    
    
    insert.exec();
    fs::create_directory(dir);
    cout << "Project  " << dir.string() << "  was created" << endl;
}
const command create_project = command("create-project","Create new project this directory: create-project <name> < . if this folder> ",twoStrF(CreateProject));

void DeleteProject(int id){
    SQLite::Statement del(db,"DELETE FROM Projects WHERE id = ?");

    del.bind(1,id);

    del.exec();

    cout << "Deleted" << endl;

}
const command delete_project = command("delete-project","Delete Project : delete-project <id>",intF(DeleteProject));



void ExportProject(int id){
    SQLite::Statement query(db,"SELECT path,title FROM Projects WHERE id = ?");
    query.bind(1,id);

    if(query.executeStep()){
       string path = query.getColumn(0);
       string title = query.getColumn(1);
       fs::path zipPath = root / (title + string(".zip"));

       string comand = "powershell Compress-Archive -Path \"" + path +  "\\*\" -DestinationPath \"" + zipPath.string() + "\"";
       system(comand.c_str());
       cout << "Export " << title << " to zip" << "\n   " << zipPath.string() << endl;
    }else{
       cout << "Not Found this project please write correct id" << endl;
    }

}
const command export_project = command("export-project","Export project to zip: < . or id >  ",intF(ExportProject));


void UpdateProject(string col,string val){

}
const command update_project = command("update-project","Update project in db: update-project <id> <WHAT UPDATE? -name for name -path for path> <new value> ",twoStrF(UpdateProject));



void init(){
    db.exec("CREATE TABLE IF NOT EXISTS Projects (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT UNIQUE NOT NULL,path TEXT UNIQUE NOT NULL) ");
    db.exec("CREATE TABLE IF NOT EXISTS Builds(id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT UNIQUE NOT NULL,version TEXT NOT NULL,has_mods BLOB DEFAULT 0,has_map BLOB DEFAULT 0,has_config BLOB DEFAULT 0) ");
    cout << "DB initialize " << endl;
    
    const char* appdata = getenv("APPDATA");
    fs::path tlauncher_p = fs::path(appdata) / ".tlauncher";
    fs::path minecraft_p = fs::path(appdata) / ".minecraft";
    if(fs::exists(tlauncher_p)){
       cout << "Tlaucnher found!!" << endl;
    }else{
       cout << "Tlaucnher not found!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
       return;
    }

    if(fs::exists(minecraft_p)){
       cout << "Minecraft found!!" << endl;
    }else{
       cout << "Minecraft not found!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
       return; 
    }
    
    ofstream file(PATHCONF);

    json j;
    j["tlauncher"] = tlauncher_p;
    j["minecraft"] = minecraft_p;
    
    file << j.dump(4);

    file.close();


}
const command init_command = command("init","initialize work this assist utility",blankF(init));



void printAllCommand(){
    cout << pwd.name << " - " << pwd.desc << endl;    
    cout << run_project.name << " - " << run_project.desc << endl;    
    cout << init_command.name << " - " << init_command.desc << endl;    
    cout << clear_project.name << " - " << clear_project.desc << endl;    
    cout << create_project.name << " - " << create_project.desc << endl;    
    cout << delete_project.name << " - " << delete_project.desc << endl;  

    cout << run_mine.name << " - " << run_mine.desc << endl;    
}
const command help_command = command("--help","all commands: \n",blankF(printAllCommand));


int main(int argc, char* argv[]){


    int vscod = system("code --version >nul 2>&1 ");
    if(vscod == 0){
       
    }else{
        cout << "Don't vscode" << endl;
        cout << "Please download vscode IDE  (Idi skachivat vscode) " << endl;
        return 1;
    }
        
    
    if(argc > 1){
        string Comm = argv[1];
    
        if(Comm == pwd.name){
          blankF f = any_cast<blankF>(pwd.action);
          f();
          return 0;
        }

        if(Comm == help_command.name){
          blankF f = any_cast<blankF>(help_command.action);
          f();
          return 0;
        
        }

        if(Comm == init_command.name){
          blankF f = any_cast<blankF>(init_command.action);
          f();
          return 0;

        }

        if(Comm == clear_project.name){
          blankF f = any_cast<blankF>(clear_project.action);
          f();
          return 0;

        }

        if(Comm == run_project.name){
          function<void(int)> f = any_cast<function<void(int)>>(run_project.action);
          if(argc == 3){
            int selected_project = stoi(argv[2]);
            f(selected_project);
          }else{
            f(-1);
          }
          return 0;

        }

        if(Comm == init_project.name){
            if(argc == 3){
               strF f = any_cast<strF>(init_project.action);    
               string title = argv[2];
               f(title);
            }else{
                cout << "Invalid arguments for this command (init-project <name> ) " << endl;
            }
          return 0;

        }

        if(Comm == list_project.name){
          blankF f = any_cast<blankF>(list_project.action);
          f();
          return 0;

        }

        if(Comm == create_project.name){
          if(argc == 3){
            string name = argv[2];
            twoStrF f = any_cast<twoStrF>(create_project.action);
            f(name,name); 
          }else if(argc == 4){
            string name = argv[2];
            string fol = argv[3];
            twoStrF f = any_cast<twoStrF>(create_project.action);
            f(name,fol); 
          }
          else{
            cout << "Invalid argument count: create-project <name> <. if this folder>" << endl;
          }
          return 0;

        }

        if(Comm == delete_project.name){
            if(argc == 3){
                int id = stoi(argv[2]);
                intF f = any_cast<intF>(delete_project.action);
                f(id);
            }else{
                cout << "Invalid argument: delete-project <id>" << endl;
            }
          return 0;

        }

        if(Comm == export_project.name){
            if(argc == 3){
                string proj = argv[2];
                intF f = any_cast<intF>(export_project.action);
                int id;
                if(proj == "."){
                    SQLite::Statement query(db,"SELECT id FROM Projects WHERE path = ?");
                    query.bind(1,root.string());
                    if(query.executeStep()){
                        id = query.getColumn(0);
                    }else{
                        cout << "Not Found this project in db" << endl;
                        return 1;
                    }

                }else{
                    try{
                        id = stoi(proj);
                    }catch(int num_expect){
                        cout << "Invalid argument: export-project <id(INTEGER!!!!INTEGER) or .>" << endl;
                        return 1;
                    }
                }
                f(id); 
            }else{
                cout << "Invalid argument: export-project <id or .>" << endl;
            }
          return 0;

        }
        
        if(Comm == update_project.name){
            
            return 0;
        }
        
        if(Comm == run_mine.name){
            intF f = any_cast<intF>(run_mine.action);
            if(argc == 3){
                try{
                   int id = stoi(argv[2]);
                   f(id);
                }catch(int num){
                    cout << "Invalid command (TЫ Daun?) try to --help" << endl;
                    return 1;
                }
            }else{
               f(-1);
            }
            return 1;
        }
        
        
        else{
            cout << "Invalid command try to --help" << endl;
        }



    }else{
        cout << "Assist utility v1.0.1" << endl;
        ofstream file(PATHCONF);
        if(file){
            cout << "Update config file" << endl;
        }else{
            cout << "Don`t found config file!!!" << endl;
        }
        file.close();
         

        
    }

    return 0;
}