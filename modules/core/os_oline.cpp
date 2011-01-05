/* OperServ core functions
 *
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

/*************************************************************************/

#include "module.h"

class CommandOSOLine : public Command
{
 public:
	CommandOSOLine() : Command("OLINE", 2, 2, "operserv/oline")
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		User *u = source.u;
		const Anope::string &nick = params[0];
		const Anope::string &flag = params[1];
		User *u2 = NULL;

		/* let's check whether the user is online */
		if (!(u2 = finduser(nick)))
			source.Reply(NICK_X_NOT_IN_USE, nick.c_str());
		else if (u2 && flag[0] == '+')
		{
			ircdproto->SendSVSO(Config->s_OperServ, nick, flag);
			u2->SetMode(OperServ, UMODE_OPER);
			u2->SendMessage(OperServ, OPER_OLINE_IRCOP);
			source.Reply(OPER_OLINE_SUCCESS, flag.c_str(), nick.c_str());
			ircdproto->SendGlobops(OperServ, "\2%s\2 used OLINE for %s", u->nick.c_str(), nick.c_str());
		}
		else if (u2 && flag[0] == '-')
		{
			ircdproto->SendSVSO(Config->s_OperServ, nick, flag);
			source.Reply(OPER_OLINE_SUCCESS, flag.c_str(), nick.c_str());
			ircdproto->SendGlobops(OperServ, "\2%s\2 used OLINE for %s", u->nick.c_str(), nick.c_str());
		}
		else
			this->OnSyntaxError(source, "");
		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(OPER_HELP_OLINE);
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "OLINE", OPER_OLINE_SYNTAX);
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(OPER_HELP_CMD_OLINE);
	}
};

class OSOLine : public Module
{
	CommandOSOLine commandosoline;

 public:
	OSOLine(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		if (!ircd->omode)
			throw ModuleException("Your IRCd does not support OMODE.");

		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(OperServ, &commandosoline);
	}
};

MODULE_INIT(OSOLine)
