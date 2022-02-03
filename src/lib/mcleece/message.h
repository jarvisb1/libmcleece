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
	inline int encrypt(const session_key& session, const mcleece::byte_view& message, const nonce& n, mcleece::byte_view& ciphertext)
	{
		if (session.key().size() < crypto_secretbox_keybytes())
			return 1;

		size_t clen = message.size() + crypto_secretbox_macbytes();
		if (ciphertext.size() < clen)
			return 2;

		int res = crypto_secretbox_easy(
		    const_cast<unsigned char*>(ciphertext.data()), message.data(), message.size(),
		    n.data(), session.key().data()
		);
		if (res != 0)
			return 3;

		ciphertext.advance(clen);
		return 0;
	}

	inline std::string encrypt(const session_key& session, const std::string& message, const nonce& n)
	{
		std::string ciphertext;
		ciphertext.resize(message.size() + crypto_secretbox_macbytes());

		mcleece::byte_view ct(ciphertext);
		unsigned res = encrypt(session, message, n, ct);
		if (res)
			return std::string();
		return ciphertext;
	}

	inline int decrypt(const session_key& session, const mcleece::byte_view& ciphertext, const nonce& n, mcleece::byte_view& message)
	{
		if (session.key().size() < crypto_secretbox_keybytes())
			return 1;

		size_t mlen = ciphertext.size() - crypto_secretbox_macbytes();
		if (message.size() < mlen)
			return 2;

		unsigned res = crypto_secretbox_open_easy(
		    const_cast<unsigned char*>(message.data()), ciphertext.data(), ciphertext.size(),
		    n.data(), session.key().data()
		);
		if (res != 0)
			return 3;

		message.advance(mlen);
		return 0;
	}

	inline std::string decrypt(const session_key& session, const std::string& ciphertext, const nonce& n)
	{
		std::string message;
		message.resize(ciphertext.size() - crypto_secretbox_macbytes());

		mcleece::byte_view mb(message);
		unsigned res = decrypt(session, ciphertext, n, mb);
		if (res)
			return std::string();
		return message;
	}

	inline constexpr unsigned session_header_size()
	{
		return session_key::size() + nonce::size();
	}

	inline bool encode_session(const session_key& session, const nonce& n, mcleece::byte_view& buff)
	{
		if (buff.size() < session.encrypted_key().size() + n.size())
			return false;

		buff.write(session.encrypted_key().data(), session.encrypted_key().size());
		buff.write(n.data(), n.size());
		return true;
	}

	inline std::string encode_session(const session_key& session, const nonce& n)
	{
		std::string buff;
		buff.resize(session.encrypted_key().size() + n.size());
		mcleece::byte_view sb(buff);
		encode_session(session, n, sb);
		return buff;
	}

	inline std::optional<std::pair<session_key, nonce>> decode_session(const unsigned char* secret, mcleece::byte_view data)
	{
		if (data.size() < session_header_size())
			return {};

		mcleece::byte_view sbuff(data.data(), session_key::size());
		auto session = mcleece::decode_session_key(secret, sbuff);
		nonce n(data.data() + session_key::size());
		return {{session, n}};
	}

	inline std::optional<std::pair<session_key, nonce>> decode_session(const private_key& secret, mcleece::byte_view data)
	{
		return decode_session(secret.data(), data);
	}
}
