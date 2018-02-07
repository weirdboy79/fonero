// Copyright (c) 2017-2018, The Fonero Project.
// Copyright (c) 2014-2017 The Monero Project.
// Portions Copyright (c) 2012-2013 The Cryptonote developers.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "include_base_utils.h"

using namespace epee;

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "include_base_utils.h"
#include <sstream>
#include <random>

#undef FONERO_DEFAULT_LOG_CATEGORY
#define FONERO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = null_hash;
    bool r = epee::string_tools::parse_tpod_from_hex_string(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    std::map< uint64_t, crypto::hash >::const_iterator highest =
        std::max_element( m_points.begin(), m_points.end(),
                         ( boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _1) <
                           boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _2 ) ) );
    return highest->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints(bool testnet)
  {
    if (testnet)
    {
      ADD_CHECKPOINT(0, "ab41bc6dbf896604021c70e30fb2d6d5c8cb9cf8c8b4fe8211438b9a46c7a8bc");
      ADD_CHECKPOINT(1, "8329e262ca4be8df9c3ebc2afcb6facb5af6848a20ab8e1e5f6dd4478508b475");
      return true;
    }
    ADD_CHECKPOINT(0, "ab41bc6dbf896604021c70e30fb2d6d5c8cb9cf8c8b4fe8211438b9a46c7a8bc");
    ADD_CHECKPOINT(1, "5d3533fd75f391b2ead7426ec1d4d2ebb46aa3a37ca68f503a8d2bb70396df9d");
    ADD_CHECKPOINT(10, "815695f5cae63ca3670e24a10db70de3475d652bbb5d3784123df515c5adabf3");
    ADD_CHECKPOINT(20, "14368744f7e5a55a481185da848518091e4a37c6b75f06af0511daa16760665c");
    ADD_CHECKPOINT(30, "ec57b6d30a23f2ce10441dbe3b607a5ada2049b0558b54002c115465f8c118d3");
    ADD_CHECKPOINT(40, "fa7fff87c8bb7fb7190b0c0e3e224dd94a75933eace2d15031b0b110737599d8");
    ADD_CHECKPOINT(50, "b766100fcc0f07913eb0f4f04edbfbd65aebeccd8c61c58ee84caf4325208152");
    ADD_CHECKPOINT(60, "35b0d81e09f5f9033c58faa6f0364bebbf626201bcdd146dd39b3c1d143f423a");
    ADD_CHECKPOINT(70, "31d68e8410094454a963881c5fb3715bc5e5a0696d82779eaec99399b23f4546");
    ADD_CHECKPOINT(80, "ae95aa5430a64ca31e750c5c57cdef654989f7dab4e23a1bcf89f72920083bff");
    ADD_CHECKPOINT(90, "ecbeb9dbbdb1e8dd6331c0529f83b0972eeadb298a50c35facac0e2ead172735");
    ADD_CHECKPOINT(100, "51f684302df08acbf6ccc6fe73819fe96ce2a6031a04f23ede728207b4f8c222");
    ADD_CHECKPOINT(200, "9f1bc5d8c1211320f2fbc491e1df48be31e9b13949d0cd72fb57a575ce668561");
    ADD_CHECKPOINT(300, "29bb8df28e58086a545f5d446b7e4982d60de35aaaf2d1ea68fd8062c977c5b1");
    ADD_CHECKPOINT(400, "79dd2c4e324d37e21b64329215961051231bfebaff8ae80f6f4e12feee17b60d");
    ADD_CHECKPOINT(500, "3ff94f87a3611d36231218b60314b5b801aa65f9bda4442136162717c0ae27b4");
    ADD_CHECKPOINT(600, "fdfa9948c8aab67b5cb010aa38f6f9091cb1e41da9cb24c023e0cf8386b63ae4");
    ADD_CHECKPOINT(700, "d963ef1aa4412191669f13e2655b12ca10e033b3d38b3a53e71f183968fb4057");
    ADD_CHECKPOINT(800, "30b48ea07a36d6a8a11eae4664f06c0607cbae299f6867c68d2ef52271d0c065");
    ADD_CHECKPOINT(900, "2d779d6675286c730e6688f74ce2ea71bc4121c10df9fb63c467ab1da8298140");
    ADD_CHECKPOINT(1000, "3874d18dc2cdf60012e92d4635e847b4677217672c54cddf1c52feb420f959cc");
    ADD_CHECKPOINT(2000, "d725fdfea957af93f2dd17d0f826d7d8185835791d6b10fe7c0e5754313c6e28");
    ADD_CHECKPOINT(3000, "1823dc7659aab71af41ffe76b960835340d623dc50b206fa00c47934304188da");
    ADD_CHECKPOINT(4000, "d28663278660467c0292836293a512b94d10a65bcc245506432e4730d934b7fd");
    ADD_CHECKPOINT(5000, "9cd06bcbac26fb66f9b9b827191785c61eba44d7754684a19f2843a699cff21e");
    ADD_CHECKPOINT(6000, "69e2f66c51be1f29566b96441576565a6e48d11e7ae221fd50ac81b02dcd9d52");
    ADD_CHECKPOINT(7000, "e1f02e1425fb23f8f48ad78e36d23c89842941014fc621bb81c67ba2a2ba6e02");
    ADD_CHECKPOINT(8000, "550022f810f4592dc35ceda27d3a4bbd6a1daedff43f8667f91ab3605971ce33");
    ADD_CHECKPOINT(9000, "a51b4c03db1b4f3f17888459f6019c1f129984c2e59e5c6cf5583262fa4af60e");
    ADD_CHECKPOINT(10000, "e123a4ce6f5216cead7d1382d5614e42cb7126e3c6d291242319700cef49d076");
    ADD_CHECKPOINT(15000, "c5f6d1c4ae0455c6c19fda6d6c6658f24ef62c6eae77192c86bc33b5ba35c39d");
    ADD_CHECKPOINT(20000, "4a8a4dc62b9424a7542b490d5eca08b2cb94493e8430fb04ffde6f7aad795943");
    ADD_CHECKPOINT(25000, "a2cee89556364c92147d95638971d51885b9a3f31bc13fb5422bff879ac1b448");
    ADD_CHECKPOINT(30000, "2eb4a3efc8c06d2eb3f5490a21a23320aa485ffda2922f8d2c896132d9229177");
    ADD_CHECKPOINT(33000, "9395aebb726ce6c1b52e39a995bf91e809b7dc85e0c78c714e15df76ec314104");
    ADD_CHECKPOINT(35000, "93fc796b153b746af226679911224b38d09ced4d037437f3c5874b4ccaae3494");
    ADD_CHECKPOINT(38000, "2b4c85ff0e12447ed93d07f269d1633b7da38515059a967a93617f518562f2cc");
    ADD_CHECKPOINT(40000, "b435c1c97afc3bf48cdd090e6504459d6b229ec54e000ee429da31deb9dd2e56");
    ADD_CHECKPOINT(45000, "c27e7a0256c0717b096f6fdfc73ebe0204deef24f4f4738951da59d6bcdcfa66");
    ADD_CHECKPOINT(50000, "a09166a039d34fcaa69945e9f49927520309e30802b01dc8fe223a30814d83a5");
    ADD_CHECKPOINT(52000, "1fe61042c5ac09b3911fdf94b596c8f472f72c63de0cc1eda9a4c2174ac508f7");
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
    LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
    t_hash_json hashes;
    epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath);
    for (std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
    {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
	LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
	std::string blockhash = it->hash;
	LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
	ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
    }

    return true;
  }

  bool checkpoints::load_checkpoints_from_dns(bool testnet)
  {
    std::vector<std::string> records;

    static const std::vector<std::string> dns_urls = {};

    static const std::vector<std::string> testnet_dns_urls = {};

    if (!tools::dns_utils::load_txt_records_from_dns(records, testnet ? testnet_dns_urls : dns_urls))
      return true; // why true ?

    for (const auto& record : records)
    {
      auto pos = record.find(":");
      if (pos != std::string::npos)
      {
        uint64_t height;
        crypto::hash hash;

        // parse the first part as uint64_t,
        // if this fails move on to the next record
        std::stringstream ss(record.substr(0, pos));
        if (!(ss >> height))
        {
    continue;
        }

        // parse the second part as crypto::hash,
        // if this fails move on to the next record
        std::string hashStr = record.substr(pos + 1);
        if (!epee::string_tools::parse_tpod_from_hex_string(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string json_hashfile_fullpath, bool testnet, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(testnet);
    }

    return result;
  }
}
