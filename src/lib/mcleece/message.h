/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "keygen.h"
#include "nonce.h"
#include "session_key.h"
#include "types.h"

#include "sodium/crypto_secretbox.h"
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace mcleece
{
	inline std::string encrypt(const session_key& session, const std::string& message, const nonce& n)
	{
		if (session.key().size() < crypto_secretbox_keybytes())
			return std::string();

		std::string ciphertext;
		ciphertext.resize(message.size() + crypto_secretbox_macbytes());
		int res = crypto_secretbox_easy(
		    reinterpret_cast<unsigned char*>(&ciphertext[0]), reinterpret_cast<const unsigned char*>(message.data()), message.size(),
		    n.data(), session.key().data()
		);
		if (res != 0)
			return std::string();
		return ciphertext;
	}

	inline std::string decrypt(const session_key& session, const std::string& ciphertext, const nonce& n)
	{
		if (session.key().size() < crypto_secretbox_keybytes())
			return std::string();

		std::string message;
		message.resize(ciphertext.size() - crypto_secretbox_macbytes());
		int res = crypto_secretbox_open_easy(
		    reinterpret_cast<unsigned char*>(&message[0]), reinterpret_cast<const unsigned char*>(ciphertext.data()), ciphertext.size(),
		    n.data(), session.key().data()
		);
		if (res != 0)
			return std::string();
		return message;
	}

	inline constexpr unsigned encoded_session_size()
	{
		return session_key::size() + nonce::size();
	}

	inline bool encode_session(const session_key& session, const nonce& n, mcleece::byte_view buff)
	{
		if (buff.size() < session.encrypted_key().size() + n.size())
			return false;

		buff.overwrite_from(session.encrypted_key().data(), session.encrypted_key().size());
		buff.overwrite_from(n.data(), n.size());
		return true;
	}

	inline std::string encode_session(const session_key& session, const nonce& n)
	{
		std::string buff;
		buff.resize(session.encrypted_key().size() + n.size());
		encode_session(session, n, buff);
		return buff;
	}

	inline std::optional<std::pair<session_key, nonce>> decode_session(const unsigned char* secret, const char* data, unsigned len)
	{
		if (len < encoded_session_size())
			return {};

		std::vector<unsigned char> sbuff(data, data + session_key::size());
		auto session = mcleece::decode_session_key(secret, sbuff);
		nonce n(data + session_key::size());
		return {{session, n}};
	}

	inline std::optional<std::pair<session_key, nonce>> decode_session(const private_key& secret, const char* data, unsigned len)
	{
		return decode_session(secret.data(), data, len);
	}

	inline std::optional<std::pair<session_key, nonce>> decode_session(const private_key& secret, const std::string& encoded)
	{
		return decode_session(secret, encoded.data(), encoded.size());
	}
}
