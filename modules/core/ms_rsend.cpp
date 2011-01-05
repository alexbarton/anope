/* MemoServ core functions
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

class CommandMSRSend : public Command
{
 public:
	CommandMSRSend() : Command("RSEND", 2, 2)
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		User *u = source.u;

		const Anope::string &nick = params[0];
		const Anope::string &text = params[1];
		NickAlias *na = NULL;

		/* prevent user from rsend to themselves */
		if ((na = findnick(nick)) && na->nc == u->Account())
		{
			source.Reply(MEMO_NO_RSEND_SELF);
			return MOD_CONT;
		}

		if (Config->MSMemoReceipt == 1)
		{
			/* Services opers and above can use rsend */
			if (u->Account()->IsServicesOper())
				memo_send(source, nick, text, 3);
			else
				source.Reply(ACCESS_DENIED);
		}
		else if (Config->MSMemoReceipt == 2)
			/* Everybody can use rsend */
			memo_send(source, nick, text, 3);
		else
		{
			/* rsend has been disabled */
			Log() << "MSMemoReceipt is set misconfigured to " << Config->MSMemoReceipt;
			source.Reply(MEMO_RSEND_DISABLED);
		}

		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(MEMO_HELP_RSEND);
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "RSEND", MEMO_RSEND_SYNTAX);
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(MEMO_HELP_CMD_RSEND);
	}
};

class MSRSend : public Module
{
	CommandMSRSend commandmsrsend;

 public:
	MSRSend(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		if (!Config->MSMemoReceipt)
			throw ModuleException("Don't like memo reciepts, or something.");

		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(MemoServ, &commandmsrsend);
	}
};

MODULE_INIT(MSRSend)
