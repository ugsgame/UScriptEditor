#include "ScriptSchemaAction.h"
#include "UScriptDebuggerSetting.h"
//#include "UnLua/Private/DefaultParamCollection.h"

#define  LOCTEXT_NAMESPACE "ScriptActionCollecter"

static FString ConcatCategories(FString RootCategory, FString const& SubCategory)
{
	FString ConcatedCategory = MoveTemp(RootCategory);
	if (!SubCategory.IsEmpty() && !ConcatedCategory.IsEmpty())
	{
		ConcatedCategory += TEXT("|");
	}
	ConcatedCategory += SubCategory;

	return ConcatedCategory;
}

UScriptActionCollecter* UScriptActionCollecter::Get()
{
	static UScriptActionCollecter* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptActionCollecter>();
		Singleton->AddToRoot();
		Singleton->LuaActionList = new FGraphActionListBuilderBase();
		Singleton->ScriptActionList = new FGraphActionListBuilderBase();

		Singleton->Reflash();
	}
	return Singleton;
}

void UScriptActionCollecter::Reflash()
{
	CreateLuaActions();
	CreateScriptActions();
}

void UScriptActionCollecter::AddScriptAction(FString InNodeCategory, FString InMenuDesc, FString InToolTip,FString InCodeClip)
{
	TSharedPtr<FScriptSchemaAction> NewAction(new FScriptSchemaAction(FText::FromString(InNodeCategory), FText::FromString(InMenuDesc), FText::FromString(InToolTip), InCodeClip));
	ScriptActions.Add(NewAction);

	ScriptActionList->AddAction(NewAction);
}

void UScriptActionCollecter::AddLuaAction(FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString InCodeClip)
{
	TSharedPtr<FScriptSchemaAction> NewAction(new FScriptSchemaAction(FText::FromString(InNodeCategory), FText::FromString(InMenuDesc), FText::FromString(InToolTip), InCodeClip));
	LuaActions.Add(NewAction);

	LuaActionList->AddAction(NewAction);
}

TArray<TSharedPtr<FEdGraphSchemaAction>> UScriptActionCollecter::GetScriptActions()
{
	return ScriptActions;
}

TArray<TSharedPtr<FEdGraphSchemaAction>> UScriptActionCollecter::GetLuaActions()
{
	return LuaActions;
}

FGraphActionListBuilderBase* UScriptActionCollecter::GetScriptActionList()
{
	return ScriptActionList;
}

FGraphActionListBuilderBase* UScriptActionCollecter::GetLuaActionList()
{
	return LuaActionList;
}

void UScriptActionCollecter::CreateScriptActions()
{
	ScriptActions.Empty();
	ScriptActionList->Empty();

// 	//
// 	for (auto& Klasses : GDefaultParamCollection)
// 	{
// 		FString ClassCategory(Klasses.Key.ToString());
// 		for (auto& Funs : (Klasses.Value).Functions)
// 		{
// 			AddScriptAction(ClassCategory, Funs.Key.ToString(), "", "");
// 		}
// 	}
// 	//

	for (auto PromptNode:UScriptDebuggerSetting::Get()->ScriptPromptArray)
	{
		AddScriptAction(PromptNode.Category, PromptNode.MenuDesc, PromptNode.ToolTip, PromptNode.CodeClip);
	}
}

void UScriptActionCollecter::CreateLuaActions()
{
	LuaActions.Empty();
	ScriptActionList->Empty();

	FString LuaCategory("LUA");

	AddLuaAction(LuaCategory, "assert",			"",			"assert(v[,message])");
	AddLuaAction(LuaCategory, "collectgarbage", "",			"collectgarbage(opt[,arg])");
	AddLuaAction(LuaCategory, "dofile",			"",			"dofile(filename)");
	AddLuaAction(LuaCategory, "error",			"",			"error(message[,level])");
	AddLuaAction(LuaCategory, "_G",				"",			"_G");
	AddLuaAction(LuaCategory, "getfenv",		"",			"getfenv(f)");
	AddLuaAction(LuaCategory, "getmetatable",	"",			"getmetatable(object)");
	AddLuaAction(LuaCategory, "load",			"",			"load(func[,chunkname])");
	AddLuaAction(LuaCategory, "loadfile",		"",			"loadfile([filename])");
	AddLuaAction(LuaCategory, "loadstring",		"",			"loadstring(string[,chunkname])");
	AddLuaAction(LuaCategory, "next",			"",			"next(table[,index])");
	AddLuaAction(LuaCategory, "ipairs",			"",			"ipairs(t)");
	AddLuaAction(LuaCategory, "pcall",			"",			"pcall(f,arg1,…)");
	AddLuaAction(LuaCategory, "print",			"",			"print(…)");
	AddLuaAction(LuaCategory, "rawequal",		"",			"rawequal(v1,v2)");
	AddLuaAction(LuaCategory, "rawget",			"",			"rawget(table,index)");
	AddLuaAction(LuaCategory, "rawset",			"",			"rawset(table,index,value)");
	AddLuaAction(LuaCategory, "select",			"",			"select(index,…)");
	AddLuaAction(LuaCategory, "setfenv",		"",			"setfenv(f,table)");
	AddLuaAction(LuaCategory, "setmetatable",	"",			"setmetatable(table,metatable)");
	AddLuaAction(LuaCategory, "tonumber",		"",			"tonumber(e[,base])");
	AddLuaAction(LuaCategory, "tostring",		"",			"tostring(e)");
	AddLuaAction(LuaCategory, "type",			"",			"type(v)");
	AddLuaAction(LuaCategory, "unpack",			"",			"unpack(list[,i[,j]])");
	AddLuaAction(LuaCategory, "_VERSION",		"",			"_VERSION");
	AddLuaAction(LuaCategory, "xpcall",			"",			"xpcall(f,err)");
	//String 
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.byte",	"",		"string.byte(s[,i[,j]])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.char",	"",		"string.char(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.dump",	"",		"string.dump(function[,strip])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.find",	"",		"string.find(s,pattern[,init[,plain]])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.format",	"",		"string.format(formatstring[,value[,…]])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.math",	"",		"string.match(s,pattern[,init])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.gmatch",	"",		"string.gmatch(s,pattern)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.gsub",	"",		"string.gsub(s,pattern,repl[,n])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.len",		"",		"string.len(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.lower",	"",		"string.lower(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.upper",	"",		"string.upper(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.rep",		"",		"string.rep(s,n[,sep])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.reverse",	"",		"string.reverse(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.sub",		"",		"string.sub(s,i[,j])");

	//Math
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.abs",			"", "math.abs(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.acos",		"", "math.acos(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.asin",		"", "math.asin(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.atan2",		"", "math.atan2(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.atan",		"", "math.atan(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.ceil",		"", "math.ceil(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.cosh",		"", "math.cosh(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.cos",			"", "math.cos(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.deg",			"", "math.deg(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.exp",			"", "math.exp(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.floor",		"", "math.floor(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.fmod",		"", "math.fmod(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.frexp",		"", "math.frexp(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.ldexp",		"", "math.ldexp(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.log10",		"", "math.log10(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.log",			"", "math.log(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.max",			"", "math.max(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.min",			"", "math.min(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.modf",		"", "math.modf(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.pow",			"", "math.pow(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.random",		"", "math.random(x[,y])");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.randomseed",	"", "math.randomseed(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.sinh",		"", "math.sinh(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.sin",			"", "math.sin(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.sqrt",		"", "math.sqrt(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.tanh",		"", "math.tanh(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.tan",			"", "math.tan(x)");

	//Table
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.concat",	"", "table.concat(table[,sep[,i[,j]]])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.insert",	"", "table.insert(table,[pos,]value)");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.move",		"", "table.move(table1,f,e,t[,table2])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.pack",		"", "table.pack(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.remove",	"", "table.remove(table[,pos])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.sort",		"", "table.sort(table[,comp])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.unpack",	"", "table.unpack(table[,i[,j]])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.getn",		"", "table.getn(table)");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.maxn",		"", "table.maxn(table)");
}

#undef LOCTEXT_NAMESPACE
