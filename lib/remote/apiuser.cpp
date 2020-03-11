/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apiuser.hpp"
#include "remote/apiuser-ti.cpp"
#include "base/configtype.hpp"
#include "base/base64.hpp"
#include "base/tlsutility.hpp"
#include "base/utility.hpp"
#include <crypt.h>
#include <unistd.h>

using namespace icinga;

REGISTER_TYPE(ApiUser);

char *crypt(const char *key, const char *salt);

ApiUser::Ptr ApiUser::GetByClientCN(const String& cn)
{
	for (const ApiUser::Ptr& user : ConfigType::GetObjectsByType<ApiUser>()) {
		if (user->GetClientCN() == cn)
			return user;
	}

	return nullptr;
}

ApiUser::Ptr ApiUser::GetByAuthHeader(const String& auth_header)
{
	String::SizeType pos = auth_header.FindFirstOf(" ");
	String username, password;

	if (pos != String::NPos && auth_header.SubStr(0, pos) == "Basic") {
		String credentials_base64 = auth_header.SubStr(pos + 1);
		String credentials = Base64::Decode(credentials_base64);

		String::SizeType cpos = credentials.FindFirstOf(":");

		if (cpos != String::NPos) {
			username = credentials.SubStr(0, cpos);
			password = credentials.SubStr(cpos + 1);
		}
	}

	const ApiUser::Ptr& user = ApiUser::GetByName(username);

	/* Deny authentication if:
	 * 1) user does not exist
	 * 2) given password is empty
	 * 2) configured password does not match.
	 */
	if (!user || password.IsEmpty())
		return nullptr;

	String hashedPassword = user->GetHashedPassword();

	if (hashedPassword.IsEmpty()) {
		if (!Utility::ComparePasswords(password, user->GetPassword()))
			return nullptr;
	} else {
		if (strcmp(hashedPassword.CStr(), crypt(password.CStr(), hashedPassword.CStr())) != 0)
			return nullptr;
	}

	return user;
}

