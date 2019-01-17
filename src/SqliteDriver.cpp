/*
 * SqliteDriver.cpp
 *
 *  Created on: Oct 10, 2018
 *      Author: connor
 */
extern "C"{
#include <sqlite.h>
};
#include <lclib/Config.hpp>
#include <lclib/database/Database.hpp>
#include <lclib/database/Exceptions.hpp>
#include <string>
#include <cstring>

using namespace db;
using namespace std::string_view_literals;
using namespace std::string_literals;

class LCLIBEXPORT SqliteRowset final:public Rowset{ // @suppress("Class has a virtual method and non-virtual destructor")
	int rows;
	std::vector<std::string> colNames;
	std::vector<std::string> values;
	int colls;
	Statement* stat;
	Connection* conn;
	int _row;
	EndRowset _end;
protected:
	bool isEnd(){
		return _row==-1||_row>=rows;
	}
	bool first(){
		_row = 0;
		return rows!=0;
	}
	bool last(){
		_row = rows-1;
		return rows!=0;
	}
	bool next(){
		_row++;
		return !isEnd();
	}
	bool previous(){
		_row--;
		return _row!=0;
	}
	int row(){
		return _row;
	}
public:
	SqliteRowset()=default;
	SqliteRowset(Connection& conn,Statement& stat,int rows,
			int colls,std::vector<std::string> collNames,
			std::vector<std::string> values):conn(&conn),stat(&stat),
			rows{rows},colls{colls},
			colNames{std::move(collNames)},values{std::move(values)},_row{0},
			_end{*this}{}
	~SqliteRowset()=default;
	Rowset& end(){
		return _end;
	}
	std::string_view getCollumName(int i){
		return colNames.at(i);
	}
	int getCollumNumber(std::string_view name){
		for(int i = 0;i<colNames.size();i++)
			if(colNames[i]==name)
				return i;
		return -1;
	}
	std::string_view getString(int i){
		return values.at(_row*colls+i);
	}
	int getInteger(int i){
		return atoi(values.at(_row*colls+i).c_str());
	}
	float getFloat(int i){
		return atof(values.at(_row*colls+i).c_str());
	}
	int64_t getLong(int i){
		return atoll(values.at(_row*colls+i).c_str());
	}
	double getDouble(int i){
		return atof(values.at(_row*colls+i).c_str());
	}
	Connection& getConnection(){
		return *conn;
	}
	Statement& getStatement(){
		return *stat;
	}
	int getRowCount(){
		return rows;
	}
};

class LCLIBEXPORT SqliteStatement final:public DirectStatement{
private:
	SqliteRowset set;
	Connection* conn;
	sqlite* db;
public:
	SqliteStatement(Connection& conn,sqlite* db):conn(&conn),db(db){}
	SqliteStatement()=default;
	Rowset& executeQuery(std::string_view query){
		std::string qstr{query};
		char** table=nullptr;
		int nrow, ncoll;
		char* errmsg;
		std::vector<std::string> colNames;
		std::vector<std::string> values;
		if(!sqlite_get_table(db,qstr.c_str(),&table,&nrow,&ncoll,&errmsg)){
			qstr = errmsg;
			sqlite_freemem(errmsg);
			if(table)
				sqlite_free_table(table);
			throw SQLException(qstr);
		}
		for(int i =0;i<ncoll;i++)
			colNames[i] = table[i];
		for(int i =0;i<ncoll*nrow;i++)
			values[i] = table[ncoll+i];
		sqlite_free_table(table);
		set = SqliteRowset{*conn,*this,nrow,ncoll,std::move(colNames),std::move(values)};
		return set;
	}
	int executeUpdate(std::string_view v){
		executeQuery(v);
		return set.getRowCount();
	}
	Connection& getConnection(){
		return *conn;
	}
};
class LCLIBEXPORT SqlitePreparedStatement final:public PreparedStatement{
private:
	std::string query;
	SqliteStatement executeStat;
	std::vector<std::string> params;
public:
	SqlitePreparedStatement(std::string_view query,Connection& conn,sqlite* db):executeStat{conn,db},query{query}{}
	SqlitePreparedStatement()=default;
	void setInt(int i,int v){
		if(params.size()<i)
			params.reserve(i);
		params[i] = std::to_string(v);
	}
	void setFloat(int i,float v){
		if(params.size()<i)
			params.reserve(i);
		params[i] = std::to_string(v);
	}
	void setDouble(int i,double v){
		if(params.size()<i)
			params.reserve(i);
		params[i] = std::to_string(v);
	}
	void setLong(int i,int64_t v){
		if(params.size()<i)
			params.reserve(i);
		params[i] = std::to_string(v);
	}
	void setString(int i,std::string_view v){
		if(params.size()<i)
			params.reserve(i);
		params[i] = v;
	}
	Rowset& executePreparedQuery(){
		std::string qstr = query;
		for(const std::string& str:params)
			qstr.replace(qstr.find("?"), 1, str);
		return executeStat.executeQuery(qstr);
	}
	int executePreparedUpdate(){
		std::string qstr = query;
		for(const std::string& str:params)
			qstr.replace(qstr.find("?"), 1, str);
		return executeStat.executeUpdate(qstr);
	}
	Connection& getConnection(){
		return executeStat.getConnection();
	}
};

class LCLIBEXPORT SqliteConnection final:public Connection{
private:
	sqlite* db=nullptr;
	SqliteStatement stat;
public:
	SqliteConnection()=default;
	~SqliteConnection(){
		if(db){
			stat.executeQuery("COMMIT");
			sqlite_close(db);
		}
	}
	void open(std::string_view uri){
		if(db)
			sqlite_close(db);
		uri.remove_prefix("sqlite:"sv.length());
		std::string name{uri};
		char* err;
		if(!(db = sqlite_open(name.c_str(),0,&err))){
			name = err;
			sqlite_freemem(err);
			throw SQLException{"SQLite Error: "s+name};
		}
		stat = SqliteStatement{*this,db};
	}
	std::unique_ptr<Statement> newStatement(){
		return std::make_unique<SqliteStatement>(*this,db);
	}
	std::unique_ptr<PreparedStatement> newPreparedStatement(std::string_view stat){
		return std::make_unique<SqlitePreparedStatement>(stat,*this,db);
	}
	void beginTransaction(){
		stat.executeQuery("BEGIN TRANSACTION");
	}
	void commit(){
		stat.executeQuery("COMMIT");
	}
	void rollback(){
		stat.executeQuery("ROLLBACK");
	}
};

class SqliteConnectionProvider final:public ConnectionProvider{
public:
	SqliteConnectionProvider(){
		registerProvider(*this);
	}
	~SqliteConnectionProvider(){
		unregisterProvider(*this);
	}
	bool supports(std::string_view uri)const{
		return uri.find("sqlite:"sv)!=std::string_view::npos;
	}
	std::unique_ptr<Connection> open(std::string_view uri)const{
		if(!supports(uri))
			return {};
		else{
			std::unique_ptr<SqliteConnection> conn = std::make_unique<SqliteConnection>();
			conn->open(uri);
			return std::move(conn);
		}
	}
};

SqliteConnectionProvider provider{};


