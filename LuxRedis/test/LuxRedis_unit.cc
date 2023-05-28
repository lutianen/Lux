#include <LuxLog/Logger.h>
#include <LuxRedis/RedisConn.h>
#include <LuxUtils/Types.h>
#include <hiredis/hiredis.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

const std::string ICON = R"Lux(

   ,--,                                
,---.'|                                
|   | :                                
:   : |                                
|   ' :             ,--,               
;   ; '           ,'_ /|   ,--,  ,--,  
'   | |__    .--. |  | :   |'. \/ .`|  
|   | :.'| ,'_ /| :  . |   '  \/  / ;  
'   :    ; |  ' | |  . .    \  \.' /   
|   |  ./  |  | ' |  | |     \  ;  ;   
;   : ;    :  | : ;  ; |    / \  \  \
|   ,/     '  :  `--'   \ ./__;   ;  \
'---'      :  ,      .-./ |   :/\  \ ; 
            `--`----'     `---'  `--`  
)Lux";

int main() {
    std::cout << ICON << std::endl;

    // --- connectSvr / disconnectorSvr / reConnectSvr
    Lux::redis::RedisConn conn;
    if (!conn.connectSvr("127.0.0.1", 6379, 1200)) {
        LOG_INFO << "Connect to redis server FAILED.";
        return -1;
    }

    conn.disconnectSvr();
    auto reply = reinterpret_cast<redisReply*>(conn.command("PING"));
    std::cout << reply->str << std::endl;
    freeReplyObject(reply);

    {
        // ---
        std::string key{"a"};
        std::string val{"av"};
        conn.setKey(key.c_str(), key.size(), val.c_str(), val.size());

        std::unique_ptr<char[]> temp(new char[128]);
        Lux::memZero(temp.get(), 128);

        std::cout << conn.getKey(key.c_str(), key.size(), temp.get(), 128)
                  << std::endl;
        std::cout << "Test getKey: " << temp.get() << std::endl;

        std::string t;
        std::cout << conn.getKey(key, t) << std::endl;
        std::cout << "Test STL getKey: " << t << std::endl;

        // ---
        std::string kb{"b"};
        std::string vb{"vb"};
        std::cout << "Test append: "
                  << conn.append(kb.c_str(), kb.size(), vb.c_str(), vb.size())
                  << std::endl;

        // ---
        std::string kc{"c"};
        std::string vc{"vcccccccc"};
        std::cout << "Test setKeyRange: "
                  << conn.setKeyRange(kc.c_str(), kc.size(), 0, vc.c_str(),
                                      vc.size())
                  << std::endl;

        // ---
        std::cout << "Test setKyeLifeTime: "
                  << conn.setKeyLifeTime(kb.c_str(),key.size(), 10) << std::endl;

        // ---
        std::unique_ptr<char[]> temp1(new char[128]);
        Lux::memZero(temp1.get(), 128);

        std::cout << conn.getKey(kc.c_str(), key.size(), temp1.get(), 128)
                  << std::endl;
        std::cout << "Test getKey: " << temp1.get() << std::endl;

        std::string t1;
        std::cout << conn.getKey(key, t1) << std::endl;
        std::cout << "Test STL getKey: " << t1 << std::endl;

        std::cout << "Test getLen: " << conn.getLen(kc.c_str(), kc.size())
                  << std::endl;

        // ---
        Lux::memZero(temp1.get(), 128);
        std::cout << "Test getkeyByRange: "
                  << conn.getKeyByRange(kc.c_str(), kc.size(), 0, 2,
                                        temp1.get(), 128)
                  << std::endl;
        std::cout << temp1.get() << std::endl;

        std::string t2;
        std::cout << conn.getKeyByRange(kc, 0, 2, t2) << std::endl;
        std::cout << "Test STL getkeyByRange: " << t2 << std::endl;

        // ---
        std::cout << "Test getKeyRemainLifeTime: "
                  << conn.getKeyRemainLifeTime(kc.c_str(), kc.size())
                  << std::endl;

        // ---
        Lux::memZero(temp1.get(), 128);
        std::cout << "Test getKeyType: "
                  << conn.getKeyType(kc.c_str(), kc.size(), temp1.get(), 128)
                  << std::endl;
        std::cout << temp1.get() << std::endl;

        std::string t3;
        std::cout << conn.getKeyType(kc, t3) << std::endl;
        std::cout << "Test STL getkeyType: " << t3 << std::endl;

        // ---
        std::cout << "Test delKey: " << conn.delKey(key.c_str(), key.size())
                  << std::endl;

        // ---
        std::cout << "Test hasKey: " << conn.hasKey(key.c_str(), key.size())
                  << std::endl;
        std::cout << "Test hasKey: " << conn.hasKey(kc.c_str(), kc.size())
                  << std::endl;

        // ---
        std::string kd{"num"};
        std::string vd{"1"};
        double vdd;
        std::cout << "Test incrByFloat: "
                  << conn.setKey(kd.c_str(), kd.size(), vd.c_str(), vd.size())
                  << std::endl;
        std::cout << "Test incrByFloat: "
                  << conn.incrByFloat(kd.c_str(), kd.size(), 0.23, vdd)
                  << std::endl;
        std::cout << "Test incrByFloat: " << vdd << std::endl;

        // ---
        std::vector<std::string> keys{"MK1", "MV1", "MK2", "MV2", "MK3", "VK3"};
        std::cout << "Test setMultiKey: " << conn.setMultiKey(keys)
                  << std::endl;
        std::vector<std::string> keys1{"MK1", "MK2", "MK3"};
        std::vector<std::string> vals;
        std::cout << "Test getMultiKey: " << conn.getMultiKey(keys1, vals)
                  << std::endl;
        std::cout << " --- " << std::endl;
        for (const auto& val : vals) {
            std::cout << val << std::endl;
        }
        std::cout << " --- " << std::endl;

        std::cout << conn.delMultiKey(keys1) << std::endl;
        std::cout << "Test getMultiKey: " << conn.getMultiKey(keys1, vals)
                  << std::endl;
        std::cout << " --- " << std::endl;
        for (const auto& val : vals) {
            std::cout << val << std::endl;
        }
        std::cout << " --- " << std::endl;
    }
    {
        // ---
        std::string lka{"la"};
        for (int i = 0; i < 2; ++i) {
            std::string v = std::to_string(i);
            conn.lpushList(lka.c_str(), lka.size(), v.c_str(), v.size());
        }

        // ---
        for (int i = 0; i < 2; ++i) {
            std::string v = std::to_string(i);
            conn.rpushList(lka.c_str(), lka.size(), v.c_str(), v.size());
        }

        // ---
        std::unique_ptr<char[]> lva(new char[64]);
        Lux::memZero(lva.get(), 64);
        std::cout << conn.lpopList(lka.c_str(), lka.size(), lva.get(), 64)
                  << std::endl;
        std::cout << "Test lpopList: " << lva.get() << std::endl;

        for (int i = 0; i < 2; ++i) {
            std::string v = std::to_string(i);
            conn.lpushList(lka.c_str(), lka.size(), v.c_str(), v.size());
        }

        std::string t0;
        std::cout << conn.lpopList(lka, t0) << std::endl;
        std::cout << "Test STL lpopList: " << t0 << std::endl;

        // ---
        Lux::memZero(lva.get(), 64);
        std::cout << conn.rpopList(lka.c_str(), lka.size(), lva.get(), 64)
                  << std::endl;
        std::cout << "Test rpopList: " << lva.get() << std::endl;

        for (int i = 0; i < 2; ++i) {
            std::string v = std::to_string(i);
            conn.lpushList(lka.c_str(), lka.size(), v.c_str(), v.size());
        }
        std::string t1;
        std::cout << conn.rpopList(lka, t1) << std::endl;
        std::cout << "Test STL rpopList: " << t1 << std::endl;

        // ---
        int index = 2;
        Lux::memZero(lva.get(), 64);
        std::cout << conn.indexList(lka.c_str(), lka.size(), index, lva.get(),
                                    64)
                  << std::endl;
        std::cout << "Test indexList: " << lva.get() << std::endl;

        std::string t2;
        std::cout << conn.indexList(lka, index, t2) << std::endl;
        std::cout << "Test STL indexList: " << t2 << std::endl;

        // ---
        std::cout << "Test lenList: " << conn.lenList(lka.c_str(), lka.size())
                  << std::endl;

        // ---
        std::vector<std::string> vstrList;
        std::cout << "Test rangeList: "
                  << conn.rangeList(lka.c_str(), lka.size(), 0, -1, vstrList)
                  << std::endl;

        for (const auto& val : vstrList) std::cout << val << std::endl;

        // ---
        index = 0;
        std::string vc{"haha"};
        std::cout << conn.setList(lka.c_str(), lka.size(), index, vc.c_str(),
                                  vc.size())
                  << std::endl;
        Lux::memZero(lva.get(), 64);
        std::cout << conn.indexList(lka.c_str(), lka.size(), index, lva.get(),
                                    64)
                  << std::endl;
        std::cout << "Test setList: " << lva.get() << std::endl;
    }

    {
        // ---
        std::string hka{"ha"};
        std::string hfa = "hfa";
        std::string hva{"123"};
        std::cout << "Test setHField: "
                  << conn.setHField(hka.c_str(), hka.size(), hfa.c_str(),
                                    hfa.size(), hva.c_str(), hva.size())
                  << std::endl;

        // ---
        std::unique_ptr<char[]> val(new char[64]);
        Lux::memZero(val.get(), 64);
        std::cout << "Test getHField: "
                  << conn.getHField(hka.c_str(), hka.size(), hfa.c_str(),
                                    hfa.size(), val.get(), 64)
                  << std::endl;

        std::cout << "Test getHField: " << val.get() << std::endl;

        std::string t0;
        std::cout << conn.getHField(hka, hfa, t0) << std::endl;
        std::cout << "Test STL getHField: " << t0 << std::endl;

        // ---
        std::cout << "Test delHField: "
                  << conn.delHField(hka.c_str(), hka.size(), hfa.c_str(),
                                    hfa.size())
                  << std::endl;
        Lux::memZero(val.get(), 64);
        conn.getHField(hka.c_str(), hka.size(), hfa.c_str(), hfa.size(),
                       val.get(), 64);

        std::cout << "Test delHField: " << val.get() << std::endl;

        // ---
        for (size_t i = 0; i < 5; ++i) {
            conn.setHField(hka.c_str(), hka.size(), hfa.c_str(), hfa.size(),
                           hva.c_str(), hva.size());
        }
        std::cout << "Test setHField: "
                  << conn.setHField(hka.c_str(), hka.size(), hfa.c_str(),
                                    hfa.size(), hva.c_str(), hva.size())
                  << std::endl;

        // ---
        std::cout << "Test hasHField: "
                  << conn.hasHField(hka.c_str(), hka.size(), hfa.c_str(),
                                    hfa.size())
                  << std::endl;

        // ---
        double dval;
        std::cout << "Test incrHByFloat: "
                  << conn.incrHByFloat(hka.c_str(), hka.size(), hfa.c_str(),
                                       hfa.size(), 1.2, dval)
                  << std::endl;
        std::cout << "Test incrHByFloat: " << dval << std::endl;

        // ---
        std::vector<std::string> vstrField;
        std::cout << "Test getAll: "
                  << conn.getHAll(hka.c_str(), hka.size(), vstrField)
                  << std::endl;
        for (const auto& field : vstrField) {
            std::cout << field << std::endl;
        }

        // ---
        std::cout << "Test getHFieldCount: "
                  << conn.getHFieldCount(hka.c_str(), hka.size()) << std::endl;

        // ---
        std::vector<std::string> vstrFields{"fa", "fb", "fc", "fd"};
        std::cout << "Test setMultiHField: "
                  << conn.setMultiHField(hka.c_str(), hka.size(), vstrFields)
                  << std::endl;
        // ---
        vstrField.clear();
        std::cout << conn.getHAll(hka.c_str(), hka.size(), vstrField)
                  << std::endl;
        for (const auto& field : vstrField) {
            std::cout << field << std::endl;
        }

        // ---
        std::vector<std::string> vstrValue;
        std::cout << "Test getMultiHField: "
                  << conn.getMultiHField(hka.c_str(), hka.size(), vstrField,
                                         vstrValue)
                  << std::endl;
        for (size_t i = 0; i < vstrField.size(); ++i) {
            std::cout << vstrField[i] << ": " << vstrValue[i] << std::endl;
        }

        // ---
        std::cout << "Test delMultiHField: "
                  << conn.delMultiHField(hka.c_str(), hka.size(), vstrField)
                  << std::endl;

        vstrField.clear();
        std::cout << conn.getHAll(hka.c_str(), hka.size(), vstrField)
                  << std::endl;
        for (const auto& field : vstrField) {
            std::cout << field << std::endl;
        }
    }
}
