/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "cbox.h"
#include "constants.h"
#include "keygen.h"
#include "simple.h"

#include "serialize/format.h"
#include "util/byte_view.h"
#include <string>
#include <sstream>
#include <vector>

// return codes attempt to match https://www.freebsd.org/cgi/man.cgi?query=sysexits
// ... emphasis on "attempt". I think of it like picking from HTTP status codes...

namespace mcleece {
namespace actions {
	static const int MAX_MESSAGE_LENGTH = 0x100000;

	inline int keypair_to_file(std::string keypath, std::string pw, int mode)
	{
		std::string pk = fmt::format("{}.pk", keypath);
		std::string sk = fmt::format("{}.sk", keypath);
		if (mode == SIMPLE)
			return mcleece::generate_keypair<SIMPLE>(pk, sk, pw);
		else // mode == CBOX
			return mcleece::generate_keypair<CBOX>(pk, sk, pw);
	}

	template <typename INSTREAM, typename OUTSTREAM>
	int encrypt(const public_key_simple& pubk, INSTREAM&& is, OUTSTREAM& os, unsigned max_length=MAX_MESSAGE_LENGTH)
	{
		if (!is)
			return 66;

		// encrypt each chunk
		std::string data;
		data.resize(max_length);

		const int header_length = mcleece::simple::MESSAGE_HEADER_SIZE;

		std::string scratch;
		scratch.resize(max_length + header_length);

		while (is)
		{
			is.read(data.data(), data.size());
			size_t last_read = is.gcount();
			if (last_read == 0)
				break;

			mcleece::byte_view buff(data.data(), last_read);
			mcleece::byte_view out(scratch);

			int res = mcleece::simple::encrypt(out, buff, pubk);
			if (res)
				return res;

			std::string_view ciphertext = {scratch.data(), last_read + header_length};
			os << ciphertext;
		}
		return 0;
	}

	template <typename INSTREAM, typename OUTSTREAM>
	int encrypt(const public_key_cbox& pubk, INSTREAM&& is, OUTSTREAM& os, unsigned max_length=MAX_MESSAGE_LENGTH)
	{
		return 1;
	}

	template <typename INSTREAM, typename OUTSTREAM>
	int encrypt(std::string keypath, INSTREAM&& is, OUTSTREAM& os, int mode, unsigned max_length=MAX_MESSAGE_LENGTH)
	{
		std::string pk = fmt::format("{}.pk", keypath);
		if (mode == SIMPLE)
		{
			mcleece::public_key pubk = mcleece::public_key_simple::from_file(pk);
			return encrypt(pubk, is, os, max_length);
		}
		else // mode == CBOX
		{
			mcleece::public_key pubk = mcleece::public_key_cbox::from_file(pk);
			return encrypt(pubk, is, os, max_length);
		}
	}

	template <typename INSTREAM, typename OUTSTREAM>
	int decrypt(const private_key_simple& secret, INSTREAM&& is, OUTSTREAM& os, unsigned max_length=MAX_MESSAGE_LENGTH)
	{
		if (!is)
			return 66;

		std::string data;
		const int header_length = mcleece::simple::MESSAGE_HEADER_SIZE;
		data.resize(max_length + header_length);

		std::string message;
		message.resize(max_length);

		// extract the message bytes
		while (is)
		{
			is.read(data.data(), data.size());
			size_t last_read = is.gcount();
			if (last_read == 0)
				break;

			mcleece::byte_view buff(data.data(), last_read);
			mcleece::byte_view out(message);

			// decrypt the message
			int res = mcleece::simple::decrypt(out, buff, secret);
			if (res)
				return res;

			std::string_view plaintext = {message.data(), last_read - header_length};
			os << plaintext;
		}
		return 0;
	}

	template <typename INSTREAM, typename OUTSTREAM>
	int decrypt(const public_key_cbox& pubk, const private_key_cbox& secret, INSTREAM&& is, OUTSTREAM& os, unsigned max_length=MAX_MESSAGE_LENGTH)
	{
		return 1;
	}

	template <typename INSTREAM, typename OUTSTREAM>
	int decrypt(std::string keypath, std::string pw, INSTREAM&& is, OUTSTREAM& os, int mode, unsigned max_length=MAX_MESSAGE_LENGTH)
	{
		std::string pk = fmt::format("{}.pk", keypath);
		std::string sk = fmt::format("{}.sk", keypath);
		if (mode == SIMPLE)
		{
			mcleece::private_key secret = mcleece::private_key_simple::from_file(sk, pw);
			return decrypt(secret, is, os, max_length);
		}
		else // mode == CBOX
		{
			mcleece::private_key secret = mcleece::private_key_cbox::from_file(sk, pw);
			mcleece::public_key pubk = mcleece::public_key_cbox::from_file(pk);
			return decrypt(pubk, secret, is, os, max_length);
		}
	}
}}
