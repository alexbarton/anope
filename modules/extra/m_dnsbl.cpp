/*
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 */

#include "module.h"

struct FakeAkill : public Command
{
	FakeAkill() : Command("AKILL", 0, 0) { this->service = OperServ; }
	CommandReturn Execute(CommandSource &, const std::vector<Anope::string> &) { return MOD_CONT; }
} fake_akill;

struct Blacklist
{
	Anope::string name;
	time_t bantime;
	Anope::string reason;
	std::map<int, Anope::string> replies;

	Blacklist(const Anope::string &n, time_t b, const Anope::string &r, const std::map<int, Anope::string> &re) : name(n), bantime(b), reason(r), replies(re) { }
};

class DNSBLResolver : public DNSRequest
{
	dynamic_reference<User> user;
	Blacklist blacklist;
	bool add_to_akill;

 public:
	DNSBLResolver(Module *c, User *u, const Blacklist &b, const Anope::string &host, bool add_akill) : DNSRequest(host, DNS_QUERY_A, true, c), user(u), blacklist(b), add_to_akill(add_akill) { }

	void OnLookupComplete(const DNSRecord *record)
	{
		if (!user || user->GetExt("m_dnsbl_akilled"))
			return;

		Anope::string record_reason;
		if (!this->blacklist.replies.empty())
		{
			sockaddrs sresult;
			sresult.pton(AF_INET, record->result);
			int result = (sresult.sa4.sin_addr.s_addr & 0xFF000000) >> 24;

			if (!this->blacklist.replies.count(result))
				return;
			record_reason = this->blacklist.replies[result];
		}

		user->Extend("m_dnsbl_akilled");

		Anope::string reason = this->blacklist.reason;
		reason = reason.replace_all_ci("%n", user->nick);
		reason = reason.replace_all_ci("%u", user->GetIdent());
		reason = reason.replace_all_ci("%g", user->realname);
		reason = reason.replace_all_ci("%h", user->host);
		reason = reason.replace_all_ci("%i", user->ip.addr());
		reason = reason.replace_all_ci("%r", record_reason);

		XLine *x = NULL;
		if (this->add_to_akill && SGLine && (x = SGLine->Add(NULL, NULL, Anope::string("*@") + user->host, Anope::CurTime + this->blacklist.bantime, reason)))
		{
			Log(LOG_COMMAND, OperServ, &fake_akill) << "for " << user->GetMask() << " (Listed in " << this->blacklist.name << ")";
			/* If AkillOnAdd is disabled send it anyway, noone wants bots around... */
			if (!Config->AkillOnAdd)
				ircdproto->SendAkill(x);
		}
		else
		{
			Log(OperServ) << "DNSBL: " << user->GetMask() << " appears in " << this->blacklist.name;
			XLine xline(Anope::string("*@") + user->host, OperServ ? OperServ->nick : "OperServ", Anope::CurTime + this->blacklist.bantime, reason);
			ircdproto->SendAkill(&xline);
		}
	}
};

class ModuleDNSBL : public Module
{
	std::vector<Blacklist> blacklists;
	bool check_on_connect;
	bool check_on_netburst;
	bool add_to_akill;

 public:
	ModuleDNSBL(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetType(SUPPORTED);

		OnReload(false);

		Implementation i[] = { I_OnReload, I_OnPreUserConnect };
		ModuleManager::Attach(i, this, 2);
	}

	void OnReload(bool)
	{
		ConfigReader config;

		this->check_on_connect = config.ReadFlag("m_dnsbl", "check_on_connect", "no", 0);
		this->check_on_netburst = config.ReadFlag("m_dnsbl", "check_on_netburst", "no", 0);
		this->add_to_akill = config.ReadFlag("m_dnsbl", "add_to_akill", "yes", 0);

		this->blacklists.clear();
		for (int i = 0, num = config.Enumerate("blacklist"); i < num; ++i)
		{
			Anope::string bname = config.ReadValue("blacklist", "name", "", i);
			if (bname.empty())
				continue;
			Anope::string sbantime = config.ReadValue("blacklist", "time", "4h", i);
			time_t bantime = dotime(sbantime);
			if (bantime < 0)
				bantime = 9000;
			Anope::string reason = config.ReadValue("blacklist", "reason", "", i);
			std::map<int, Anope::string> replies;
			for (int j = 0; j < 256; ++j)
			{
				Anope::string k = config.ReadValue("blacklist", stringify(j), "", i);
				if (!k.empty())
					replies[j] = k;
			}

			this->blacklists.push_back(Blacklist(bname, bantime, reason, replies));
		}
	}

	EventReturn OnPreUserConnect(User *u)
	{
		if (!this->check_on_connect && !Me->IsSynced())
			return EVENT_CONTINUE;
		if (!this->check_on_netburst && !u->server->IsSynced())
			return EVENT_CONTINUE;
		/* At this time we only support IPv4 */
		if (u->ip.sa.sa_family != AF_INET)
			return EVENT_CONTINUE;

		unsigned long ip = u->ip.sa4.sin_addr.s_addr;
		unsigned long reverse_ip = ((ip & 0xFF) << 24) | ((ip & 0xFF00) << 8) | ((ip & 0xFF0000) >> 8) | ((ip & 0xFF000000) >> 24);

		sockaddrs user_ip;
		user_ip.sa4.sin_family = AF_INET;
		user_ip.sa4.sin_addr.s_addr = reverse_ip;

		for (unsigned i = 0; i < this->blacklists.size(); ++i)
		{
			const Blacklist &b = this->blacklists[i];

			try
			{
				Anope::string dnsbl_host = user_ip.addr() + "." + b.name;
				DNSBLResolver *res = new DNSBLResolver(this, u, b, dnsbl_host, this->add_to_akill);
				res->Process();
			}
			catch (const SocketException &ex)
			{
				Log() << "m_dnsbl: " << ex.GetReason();
			}
		}

		return EVENT_CONTINUE;
	}
};

MODULE_INIT(ModuleDNSBL)

