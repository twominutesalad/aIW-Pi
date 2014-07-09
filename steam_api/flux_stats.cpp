/*
================================
Class for exporting player stats in XML format
Exports to stats.xml
================================
*/

/*
NOTES:
- Nothing here

TODO:
- Rewrite in non-CLI C++
- Send output to flux
*/

#include "StdAfx.h"

void Logger(unsigned int lvl, char* caller, char* logline, ...);

using namespace System;
using namespace System::Collections;
using namespace System::IO;

bool isValidArg(String^ arg)
{
    if (/*arg->Contains("deathstreak") || arg->Contains("inUse") || arg->Contains("name") || arg->Contains("specialGren") || arg->Contains("camo") || arg->Contains("weapon") || */arg->Contains("award") || arg->Contains("value"))
    {
        return false;
    }
    return true;
}

void parseFile(char* statz)
{
	Logger(lINFO, "alterIWnet", "Exporting player stats file");

	String^ stats = gcnew System::String(statz);

	ArrayList^ args = gcnew ArrayList();
	ArrayList^ vals = gcnew ArrayList();
	ArrayList^ cargs = gcnew ArrayList();
	ArrayList^ cvals = gcnew ArrayList();

	StringReader^ reader = gcnew StringReader(stats);

	int lineNo = 0;
	int wpCount = 0;
	int clsCount = 0;

	String^ line;
	String^ cClasses = "  <customClasses>\n";

	bool inPerks = false;
	bool inWeaps = false;
	bool inAttach = false;
	bool cClass = false;

	while ((line = reader->ReadLine()) != nullptr)
	{
		if (line->Contains("customClasses"))
		{
			cClass = true;
			continue;
		}
		if (line->Contains("deathStreak"))
		{
			cClasses += "    </class>\n";
			cClasses += "  </customClasses>\n";
			cClass = false;
		}
		if (cClass)
		{
			if ( line->Contains("perks") )
			{
				inPerks = true;
				inWeaps = false;
				inAttach = false;
				cClasses += "      <perks>\n";
				continue;
			}
			else if ( line->Contains("weaponSetups") )
			{
				inPerks = false;
				inWeaps = true;
				inAttach = false;
				cClasses += "      <weapons>\n";
				continue;
			}

			if ( line->Contains("specialGrenade") )
			{
				inPerks = false;
				cClasses += "      </perks>\n";
			}

			if ( inPerks && line->Contains("=") )
			{
				cli::array<String^>^ newStrings = line->Split('=');
				String^ val = newStrings[1]->Replace(" ", "");
				cClasses += "        <perk>" + val + "</perk>\n";
			}

			if ( inWeaps )
			{
				if (!line->Contains("="))
				{
					String^ arg = line->Replace(" ", "");
					arg = arg->Replace("	", "");
					if ( arg == "[0]" )
					{
						cClasses += "        <primary>\n";
					}
					else if ( arg == "[1]" )
					{
						cClasses += "        <secondary>\n";
					}
					else if ( arg == "attachment" )
					{
						cClasses += "          <attachments>\n";
						inAttach = true;
					}
				}
				else
				{
					cli::array<String^>^ newStrings = line->Split('=');
					String^ arg = newStrings[0]->Replace(" ", "");
					arg = arg->Replace("	", "");
					String^ val = newStrings[1]->Replace(" ", "");
					if ( inAttach )
					{
						if ( arg == "camo" )
						{
							cClasses += "          </attachments>\n";
							cClasses += "          <" + arg + ">" + val + "</" + arg + ">\n";
							inAttach = false;
						}
						else
						{
							cClasses += "            <attach>" + val + "</attach>\n";
						}
					}
					else
					{
						if ( arg == "weapon" && inWeaps )
						{
							cClasses += "          <" + arg + ">" + val + "</" + arg + ">\n";
							if ( wpCount == 1 )
							{
								wpCount = 0;
								cClasses += "        </secondary>\n";
								cClasses += "      </weapons>\n";
								inWeaps = false;
							}
							else
							{
								cClasses += "        </primary>\n";
								wpCount += 1;
							}
						}
						else
						{
							cClasses += "        <" + arg + ">" + val + "</" + arg + ">\n";
						}
					}
				}
			}

			if ( line->Contains("=") && !inPerks && !inWeaps && !inAttach )
			{
				cli::array<String^>^ newStrings = line->Split('=');
				String^ arg = newStrings[0]->Replace(" ", "");
				arg = arg->Replace("	", "");
				String^ val = newStrings[1]->Replace(" ", "");
				if ( arg != "weapon")
				{
					cClasses += "      <" + arg + ">" + val + "</" + arg + ">\n";
				}
			}

			if ( line->Contains("[") && !inPerks && !inWeaps && !inAttach )
			{
				if ( clsCount > 0 )
				{
					cClasses += "    </class>\n";
				}
				if ( clsCount < 10 )
				{
					cClasses += "    <class>\n";
				}
				clsCount += 1;
			}
		}
		else
		{
			if (!line->Contains("[") && line->Contains("="))
			{
				cli::array<String^>^ newStrings = line->Split('=');
				String^ arg = newStrings[0]->Replace(" ", "");
				arg = arg->Replace("	", "");
				String^ val = newStrings[1]->Replace(" ", "");
				if (isValidArg(arg))
				{
					bool exists = false;
					for each (String^ str in args)
					{
						if (str == arg)
						{
							exists = true;
						}
					}
					if (!exists)
					{
						args->Add(arg);
						vals->Add(val);
					}
				}
			}
		}
		
		StringReader^ reader2 = gcnew StringReader(cClasses);

		StreamWriter^ sw = gcnew StreamWriter("stats.xml");
		sw->WriteLine("<stats>");
		for (size_t e = 0; e < args->Count; e++)
		{
			sw->WriteLine("  <" + args[e] + ">" + vals[e] + "</" + args[e] + ">");
		}
		while ((line = reader2->ReadLine()) != nullptr)
		{
			sw->WriteLine(line);
		}
		sw->WriteLine("</stats>");
		sw->Flush();
		sw->Close();
	}
}