{"FF":"Default","A":[{"comment":"Loads and runs a script which check a GitHub repository's latest published Release and compares it to a current version number which is passed in the function arguments.\n`checkReleaseVersion()` arguments are:\n1. The repository's partial URL path consisting of the owners user name and the repository name. In this case it is checking this plugin's releases.\n2. The current version number to compare against.  In this case it is using a special variable which is provided by this plugin to indicate the currently running version.\n3. Specify whether to show a Notification in Touch Portal if a newer version is detected.  if false, then the result is only logged to the console.log file.\n\nThis function does not return any result immediately because it uses a background network request to do the version check, and the function itself returns right away.\nTherefore this can just as well be run as a \"one time\" standalone script (without creating a new State/instance).  \n\nWhen launched this way, It must run in the Shared environment, otherwise the instance will be deleted before the actual network request can complete (as opposed to the\nother action types which can create persistent Private environments).","KEY_TYPE":"COMMENT_ACTION"},{"kPlugType":2,"kID":"us.paperno.max.tpp.dsep.act.script.oneshot","kPrefix":"Dynamic Script Engine","kInline":"false","kHHF":"false","kcD":-14863547,"kPID":"us.paperno.max.tpp.dsep","kData":[{"default":"Expression","id":"us.paperno.max.tpp.dsep.act.script.oneshot.type","label":"Script Type","valueChoices":["Expression","Script","Module"],"type":"choice"},{"default":"","id":"us.paperno.max.tpp.dsep.act.script.oneshot.expr","label":"Expression","type":"text"},{"default":"","extensions":[],"id":"us.paperno.max.tpp.dsep.act.script.oneshot.file","label":"Script File","type":"file"},{"default":"M","id":"us.paperno.max.tpp.dsep.act.script.oneshot.alias","label":"Module Alias","type":"text"},{"default":"Shared","id":"us.paperno.max.tpp.dsep.act.script.oneshot.scope","label":"Engine Instance","valueChoices":["Shared","Private"],"type":"choice"}],"kVals":[{"VAL":"Script","ID":"us.paperno.max.tpp.dsep.act.script.oneshot.type","TYPE":"choice"},{"VAL":"checkReleaseVersion(\"mpaperno/DSEP4TP\", DSE.PLUGIN_VERSION_STR, true)   ","ID":"us.paperno.max.tpp.dsep.act.script.oneshot.expr","TYPE":"text"},{"VAL":"../examples/FetchAndNotify/check_gh_releases.js   ","ID":"us.paperno.max.tpp.dsep.act.script.oneshot.file","TYPE":"file"},{"VAL":"","ID":"us.paperno.max.tpp.dsep.act.script.oneshot.alias","TYPE":"text"},{"VAL":"Shared","ID":"us.paperno.max.tpp.dsep.act.script.oneshot.scope","TYPE":"choice"}],"kStatic":"false","kcL":-13609354,"kDesc":"Dynamic Script Engine: Run a One-Time/Anonymous Expression, Script or Module.\nThis action does not create a State. It is meant for running code which does not return any value. Otherwise can be used the same as \"Evaluate,\" \"Load\" and \"Module\" actions.","kET":0,"KEY_TYPE":"PLUGIN_ACTION","kFormat":"Action\nType{$us.paperno.max.tpp.dsep.act.script.oneshot.type$} Evaluate\nExpression{$us.paperno.max.tpp.dsep.act.script.oneshot.expr$} Script/Module\nFile (if any){$us.paperno.max.tpp.dsep.act.script.oneshot.file$} Module\nalias (if req'd){$us.paperno.max.tpp.dsep.act.script.oneshot.alias$} Engine\nInstance{$us.paperno.max.tpp.dsep.act.script.oneshot.scope$}","kName":"Anonymous (One-Time) Script"}],"BD":1,"C":[],"BE":-13684944,"kSCM":25,"BG":-13684944,"E":[],"kIAPBKC":-14803426,"I":"","ITS":true,"BiR":true,"kSCTY":0,"BiT":true,"kSCHS":false,"inS":"","IiS":true,"T":"CHECK FOR\nPLUGIN\nUPDATES","kSCAC":-14145496,"kSCC":-4611631,"kSCHRC":false,"THO":4,"id":"u610lln938csk","GUdata":"","kSCIUFATS":false,"kCT":1,"kSIP":0,"TELS":5,"kSCI":"","kIAs":[],"GUid":-1,"kSCIIVA":true,"COLS":1,"TA":5,"TC":-1,"kSVP":0,"kSTP":0,"kSVAC":-10575407,"TO":4,"TP":2,"inB":false,"EXP":[],"kSD":0,"kSCTM":0,"TS":-1,"inC":0,"ROWS":1}