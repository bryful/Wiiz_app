#pragma once
#include "spell_support.h"
#include "wizardry_item_text.h"
#include <functional>

// Native inventory formats are normalized only inside this paused editor.
// Commit repacks each title and uses its existing save/checksum path.
class ItemMenu {
    std::vector<char> rom_;
    std::vector<SpellCharacter> members_;
    using Record=std::array<unsigned char,256>;
    static constexpr unsigned inventory=128,capacity=8;
    struct Profile {GameId id;unsigned roster,work,stride,party,bank,recalc,table,itemSize,itemCount,textBank,shop,tavern,camp;};
    Profile p_{};
    bool color()const{return p_.stride==110;}
    bool third()const{return p_.id==GameId::Gaiden3;}
    void unpack(Record& rec)const{
        for(unsigned i=0;i<8;i++){
            if(color()){
                unsigned f=rec[0x42+i];rec[inventory+i*2]=rec[0x4a+i];
                rec[inventory+i*2+1]=static_cast<unsigned char>(((f&32)?1:0)|((f&128)?2:0)|((f&64)?4:0)|((f&31)<<3));
            }else{rec[inventory+i*2]=rec[0x1d+i*2];rec[inventory+i*2+1]=rec[0x1e + i*2];}
        }
    }
    void pack(Record& rec)const{
        unsigned n=0;
        for(unsigned i=0;i<8;i++){
            unsigned id=rec[inventory+i*2],f=rec[inventory+i*2+1];if(id!=255)++n;
            if(color()){rec[0x4a+i]=static_cast<unsigned char>(id);rec[0x42+i]=static_cast<unsigned char>(((f&1)?32:0)|((f&2)?128:0)|((f&4)?64:0)|(f>>3));}
            else{rec[0x1d+i*2]=static_cast<unsigned char>(id);rec[0x1e + i*2]=static_cast<unsigned char>(f);}
        }
        if(color())rec[0x52]=static_cast<unsigned char>(n);
    }
    Record loadRecord(gambatte::GB& core,unsigned slot)const{
        Record result{};auto r=static_cast<const unsigned char*>(core.rambank0_ptr())+p_.roster+slot*p_.stride;
        std::copy(r,r+p_.stride,result.begin());unpack(result);return result;
    }
    unsigned character_[2]{0,1},row_[2]{0,0},focus_=0,subRow_=0;
    bool submenu_=false,deleting_=false,yes_=false,supported_=false;
    std::string message_;
    static size_t field(const std::vector<char>& state,const std::string& key){
        if(state.size()<5)return 0;
        size_t p=5+(static_cast<unsigned char>(state[2])<<16)+(static_cast<unsigned char>(state[3])<<8)+static_cast<unsigned char>(state[4]);
        while(p<state.size()){
            size_t q=p;while(q<state.size()&&state[q])++q;
            if(q+4>state.size())return 0;
            unsigned n=(static_cast<unsigned char>(state[q+1])<<16)|(static_cast<unsigned char>(state[q+2])<<8)|static_cast<unsigned char>(state[q+3]);
            if(n>state.size()-q-4)return 0;
            if(std::string(state.data()+p,q-p)==key)return q+4;p=q+4+n;
        }return 0;
    }
    Record record(gambatte::GB& core,unsigned side)const{
        return loadRecord(core,members_[character_[side]].slot);
    }
    static unsigned count(const Record& rec){unsigned n=0;while(n<capacity&&rec[inventory+n*2]!=255)++n;return n;}
    static void erase(Record& rec,unsigned index){
        for(unsigned i=index;i+1<capacity;i++){rec[inventory+i*2]=rec[inventory+i*2+2];rec[inventory+i*2+1]=rec[inventory+i*2+3];}
        rec[inventory+14]=255;rec[inventory+15]=0;
    }
    unsigned byte(size_t at)const{return at<rom_.size()?static_cast<unsigned char>(rom_[at]):255;}
    unsigned item(unsigned id,unsigned offset)const{
        size_t at=p_.table+id*p_.itemSize;
        if(color()){unsigned ptr=byte(at)|(byte(at+1)<<8),bank=byte(at+2);if(ptr<0x4000||ptr>=0x8000||bank==255)return 255;at=bank*0x4000+ptr-0x4000;}
        return byte(at+offset);
    }
    std::string itemName(unsigned id,unsigned flags)const{
        if(!color()){
            unsigned off=(flags&1)?0:2,ptr=item(id,off)|(item(id,off+1)<<8);
            size_t at=p_.textBank*0x4000+ptr-0x4000;
            return ptr>=0x4000&&ptr<0x8000&&at+24<rom_.size()?SpellSupport::name(reinterpret_cast<const unsigned char*>(rom_.data()+at),24):"未確認";
        }
        auto name=WizardryItemText::decode(rom_,id,(flags&1)!=0);
        return name.empty()?"未確認":name;
    }
    bool valid(const Record& rec)const{
        bool empty=false;
        for(unsigned i=0;i<capacity;i++){
            unsigned id=rec[inventory+i*2];if(id==255){empty=true;continue;}
            if(empty||id>=p_.itemCount||(!color()&&(rec[inventory+i*2+1]&~7u)))return false;
        }return true;
    }
    // Run only the game's existing non-interactive recalculation on an isolated
    // core. Merge its character record, never its CPU, screen or clock state.
    bool recalculate(gambatte::GB& core,Record& rec)const{
        struct NoInput:gambatte::InputGetter{unsigned operator()()override{return 0;}} input;
        auto clone=loadCoreBytes(rom_,&input);pack(rec);
        std::vector<char> state(core.stateSize());core.saveState(state.data());
        auto ram=field(state,"wram"),hram=field(state,"hram"),pc=field(state,"pc"),sp=field(state,"sp"),bank=field(state,"rombank"),ime=field(state,"ime"),halt=field(state,"halt"),a=field(state,"a");
        if(!ram||!hram||!pc||!sp||!bank||!ime||!halt||!a)return false;
        std::copy(rec.begin(),rec.begin()+p_.stride,state.begin()+ram+p_.work);
        unsigned nativeBank=color()?4:2;state[bank]=0;state[bank+1]=static_cast<char>(nativeBank);state[ram+p_.bank]=static_cast<char>(nativeBank);
        if(color()){
            state[ram+0xa97]=static_cast<char>(p_.work+0xc000);state[ram+0xa98]=static_cast<char>((p_.work+0xc000)>>8);
            state[hram+0x170]=1;state[hram+0x1b0]=0;
        }
        state[pc]=char(0xff);state[pc+1]=char(0x80);state[sp]=char(color()?0xcf:0xdf);state[sp+1]=char(0xe0);state[ime]=0;state[halt]=0;state[a]=1;
        // HRAM state starts at FE00. Disable interrupts before native work;
        // completion loops in HRAM with a separate marker at FF90.
        const unsigned char code[]={0xf3,0xcd,static_cast<unsigned char>(p_.recalc),static_cast<unsigned char>(p_.recalc>>8),0x3e,0xa5,0xe0,0x90,0x18,0xfe};
        for(unsigned i=0;i<sizeof(code);i++)state[hram+0x180+i]=char(code[i]);state[hram+0x190]=0;
        state[hram+0x1ff]=0;
        if(!clone->loadState(state.data(),state.size()))return false;
        std::array<gambatte::video_pixel_t,160*144> video{};std::array<gambatte::uint_least32_t,40000> audio{};
        for(unsigned i=0;i<3;i++){
            unsigned samples=35112;clone->runFor(video.data(),160,audio.data(),audio.size(),samples);
            if(static_cast<const unsigned char*>(clone->zeropage_ptr())[0x10]==0xa5){
                auto r=static_cast<const unsigned char*>(clone->rambank0_ptr())+p_.work;std::copy(r,r+p_.stride,rec.begin());unpack(rec);return valid(rec);
            }
        }return false;
    }
    bool commit(gambatte::GB& core,SpellSupport& spells,const std::vector<std::pair<unsigned,Record>>& changes){
        auto computed=changes;
        // Some items grant passive effects while carried. Transfers, deletion
        // and bulk actions must also rebuild both characters' derived stats.
        for(auto& change:computed)if(!valid(change.second)||!recalculate(core,change.second)){message_="能力更新に失敗しました";return false;}
        std::vector<char> backup(core.stateSize());core.saveState(backup.data());
        for(const auto& change:computed){
            unsigned index=change.first;Record before=loadRecord(core,members_[index].slot);
            if(!valid(change.second)||!spells.replaceItemRecord(core,members_[index].slot,before.data(),change.second.data())){
                core.loadState(backup.data(),backup.size());spells.resync(core);message_="更新できません";return false;
            }
        }
        if(color()){auto r=static_cast<unsigned char*>(core.rambank0_ptr());for(const auto& change:computed)r[0x1300+change.first]=change.second[0x5f];}
        spells.resync(core);return true;
    }
    void clamp(gambatte::GB& core){for(unsigned side=0;side<2;side++)if(row_[side]<8)row_[side]=std::min(row_[side],count(record(core,side))?count(record(core,side))-1:0u);}
    void execute(gambatte::GB& core,SpellSupport& spells,unsigned action){
        auto rec=record(core,focus_);unsigned row=row_[focus_],n=count(rec),off=inventory+row*2;
        if(row>=n)return;
        unsigned flags=rec[off+1];
        if(action==0){
            if(flags&4){message_="呪われています";return;}
            if(flags&2)rec[off+1]&=~2;
            else{
                unsigned id=rec[off],type=item(id,color()?0:4);
                unsigned race=color()?rec[10]>>5:rec[8],job=color()?(rec[10]>>2)&7:rec[9],alignment=color()?rec[10]&3:rec[10];
                unsigned jobs=color()?item(id,11):third()?(item(id,21)|(item(id,22)<<8)):item(id,20);
                unsigned raceAlign=color()?item(id,1):third()?(item(id,5)|(item(id,6)<<8)):item(id,5);
                bool allowed=!(flags&1)&&type!=5&&type<=6&&job<(third()?12u:8u)&&alignment<=2&&(jobs&(1u<<job));
                if(third()){
                    unsigned sex=(item(id,6)>>5)&3;
                    allowed=allowed&&race<10&&(raceAlign&(8u<<race))&&(!sex||sex==unsigned(rec[0x69])+1);
                }else if(!color())allowed=allowed&&race<5&&(raceAlign==255||(!(raceAlign&7)||(raceAlign&(1u<<alignment))))&&(raceAlign==255||!(raceAlign>>3)||((raceAlign>>3)&(1u<<race)));
                else allowed=allowed&&((raceAlign&7)&(1u<<alignment));
                if(!allowed){message_="装備できません";return;}
                for(unsigned i=0;i<n;i++)if((rec[inventory+i*2+1]&2)&&item(rec[inventory+i*2],color()?0:4)==type){
                    if(rec[inventory+i*2+1]&4){message_="呪われています";return;}rec[inventory+i*2+1]&=~2;
                }
                rec[off+1]|=2;
                if(color()&&item(id,2)==255)rec[off+1]|=4;
            }
        }else if(action==1){
            if(flags&6){message_="装備を外してください";return;}
            auto other=record(core,1-focus_);unsigned dest=count(other);if(dest==capacity||character_[0]==character_[1]){message_="移動先に空きがありません";return;}
            other[inventory+dest*2]=rec[off];other[inventory+dest*2+1]=flags;erase(rec,row);
            if(commit(core,spells,{{character_[focus_],rec},{character_[1-focus_],other}}))message_="移動しました";clamp(core);return;
        }else if(action==2){if(flags&6){message_="装備を外してください";return;}erase(rec,row);}
        else{
            if((action==3&&row==0)||(action==4&&row+1>=n))return;
            unsigned dest=action==3?row-1:row+1;std::swap(rec[off],rec[inventory+dest*2]);std::swap(rec[off+1],rec[inventory+dest*2+1]);
            if(commit(core,spells,{{character_[focus_],rec}}))row_[focus_]=dest;return;
        }
        if(commit(core,spells,{{character_[focus_],rec}}))message_="変更しました";clamp(core);
    }
    void bulk(gambatte::GB& core,SpellSupport& spells,bool collect){
        unsigned destination=collect?focus_:1-focus_;auto dest=record(core,destination);unsigned n=count(dest),moved=0;
        std::vector<std::pair<unsigned,Record>> changes;
        for(unsigned member=0;member<members_.size()&&n<capacity;member++){
            if(member==character_[destination]||(!collect&&member!=character_[focus_]))continue;
            Record src=loadRecord(core,members_[member].slot);bool changed=false;
            for(unsigned i=0;i<count(src)&&n<capacity;){
                unsigned off=inventory+i*2,flags=src[off+1];
                if(!(flags&6)&&(!collect||(flags&1))){dest[inventory+n*2]=src[off];dest[inventory+n*2+1]=flags;++n;++moved;erase(src,i);changed=true;}else ++i;
            }if(changed)changes.push_back({member,src});
        }
        if(!moved){message_="移動できる品がありません";return;}
        changes.push_back({character_[destination],dest});if(commit(core,spells,changes))message_=std::to_string(moved)+"個 移動しました";clamp(core);
    }
public:
    struct Line{int x,y,w,h;std::string text;bool selected=false;};
    bool open=false;
    void attach(const std::vector<char>& rom){
        rom_=rom;open=false;supported_=false;p_={};
        static const Profile profiles[]={
            {GameId::Gaiden1,0x1186,0x1104,128,0x94f,0x10ae,0x5d7b,0x1c000,37,138,1,0x4074,0x695c,0x512c},
            {GameId::Gaiden2,0x1095,0x1013,128,0xd28,0xfbd,0x598e,0x1c000,37,158,1,0x4074,0x655a,0x532f},
            {GameId::Gaiden3,0xba1,0xb1f,128,0x9ae,0xac7,0x5b6f,0x1828e,40,224,1,0x40be,0x6844,0x5cb0},
            {GameId::Wizardry1,0x10a,0x9a2,110,0x109,0x49,0x42f1,0x24000,3,191,10,0x6ba4,0x575d,0x40d8},
            {GameId::Wizardry2,0x10a,0x9a2,110,0x109,0x49,0x42f1,0x24000,3,192,10,0x6b95,0x574e,0x40d8},
            {GameId::Wizardry3,0x10a,0x9a2,110,0x109,0x49,0x42f1,0x24000,3,226,10,0x6ba4,0x575d,0x40d8}
        };
        for(const auto& p:profiles)if(identifyGame(rom).id==p.id){p_=p;break;}
        if(!p_.stride||!SpellSupport::recognizes(rom)||rom.size()<0x30000)return;
        size_t routine=(color()?0x10000:0x8000)+p_.recalc-0x4000;
        if(color())supported_=byte(routine)==0xfa&&byte(routine+1)==0x70&&byte(routine+2)==0xff&&byte(routine+9)==0xfa&&byte(routine+10)==0x97&&byte(routine+11)==0xca;
        else supported_=byte(routine)==0xea&&byte(routine+3)==0xfa&&byte(routine+4)==((p_.work+0xc000+0x13)&255)&&byte(routine+5)==((p_.work+0xc000+0x13)>>8)&&byte(routine+6)==0x6f;
    }
    bool available(gambatte::GB& core,SpellSupport& spells)const{
        if(!supported_||!spells.available(core))return false;
        // The roster and party are shared across dungeon, town, facilities
        // and training. Scene/return-address allowlists wrongly excluded shops.
        // SpellSupport already excludes battle and interrupted data transfers.
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr());
        return r[p_.party]>=1&&r[p_.party]<=6;
    }
    bool show(gambatte::GB& core,SpellSupport& spells){
        open=false;if(!available(core,spells))return false;
        auto r=static_cast<const unsigned char*>(core.rambank0_ptr());auto roster=spells.characters(core);members_.clear();
        for(unsigned i=0;i<r[p_.party];i++){
            unsigned slot=r[(color()?0x103:p_.party+1)+i]&31;auto it=std::find_if(roster.begin(),roster.end(),[&](const auto& c){return c.slot==slot;});
            if(it==roster.end()||std::any_of(members_.begin(),members_.end(),[&](const auto& c){return c.slot==slot;}))return false;members_.push_back(*it);
        }
        character_[0]=0;character_[1]=members_.size()>1?1:0;row_[0]=row_[1]=focus_=subRow_=0;submenu_=deleting_=false;message_.clear();
        for(unsigned i=0;i<members_.size();i++){
            Record rec=loadRecord(core,members_[i].slot);if(!valid(rec)||(color()&&rec[0x52]!=count(rec)))return false;
        }open=true;return true;
    }
    bool key(gambatte::GB& core,SpellSupport& spells,unsigned bits){
        if(!open)return false;
        if(!available(core,spells)){open=false;return false;}
        if(deleting_){
            if(bits&2){deleting_=false;return true;}if(bits&(16|32|64|128))yes_=!yes_;
            if(bits&1){deleting_=false;submenu_=false;if(yes_)execute(core,spells,2);}return true;
        }
        if(submenu_){
            if(bits&2){submenu_=false;return true;}
            if(bits&64)subRow_=(subRow_+4)%5;else if(bits&128)subRow_=(subRow_+1)%5;
            else if(bits&1){if(subRow_==2){deleting_=true;yes_=false;}else{submenu_=false;execute(core,spells,subRow_);}}return true;
        }
        if(bits&2){open=false;return false;}
        message_.clear();if(bits&32)focus_=0;else if(bits&16)focus_=members_.size()>1?1:0;
        else if(bits&(256|512)){
            if(members_.size()>2){unsigned next=character_[focus_];do{next=(next+members_.size()+((bits&256)?-1:1))%members_.size();}while(next==character_[1-focus_]);character_[focus_]=next;row_[focus_]=0;}
        }else if(bits&(64|128)){
            std::vector<unsigned> rows;for(unsigned i=0;i<count(record(core,focus_));i++)rows.push_back(i);rows.push_back(8);rows.push_back(9);
            auto at=std::find(rows.begin(),rows.end(),row_[focus_]);unsigned i=at==rows.end()?0:unsigned(at-rows.begin());row_[focus_]=rows[(i+rows.size()+((bits&64)?-1:1))%rows.size()];
        }else if(bits&1){if(row_[focus_]>=8)bulk(core,spells,row_[focus_]==8);else if(row_[focus_]<count(record(core,focus_))){submenu_=true;subRow_=0;}}
        return open;
    }
    void tap(gambatte::GB& core,SpellSupport& spells,int x,int y){
        if(!open)return;
        if(members_.size()==1&&x>=80&&y<98)return;
        if(deleting_){if(x>=20&&x<140&&y>=66&&y<82){yes_=x<80;key(core,spells,1);}return;}
        if(submenu_){if(x>=30&&x<130&&y>=35&&y<95){subRow_=unsigned(y-35)/12;key(core,spells,1);}return;}
        if(y<16){focus_=x<80?0:1;if(x%80<12)key(core,spells,256);else if(x%80>=68)key(core,spells,512);return;}
        if(y>=18&&y<98){focus_=x<80?0:1;unsigned row=unsigned(y-18)/10;if(row<count(record(core,focus_))){row_[focus_]=row;key(core,spells,1);}return;}
        if(y>=100&&y<124){row_[focus_]=8+(y>=112);key(core,spells,1);}
    }
    std::vector<Line> lines(gambatte::GB& core)const{
        std::vector<Line> out;if(!open)return out;
        for(unsigned side=0;side<2;side++){
            if(side==1&&members_.size()==1){out.push_back({82,2,76,12,"相手なし"});continue;}
            int x=side*80;out.push_back({x+2,2,10,12,"<"});out.push_back({x+14,2,52,12,members_[character_[side]].name,focus_==side});out.push_back({x+68,2,10,12,">"});auto rec=record(core,side);
            for(unsigned i=0;i<count(rec);i++){
                unsigned id=rec[inventory+i*2],flags=rec[inventory+i*2+1];std::string name=itemName(id,flags);
                out.push_back({x+2,int(18+i*10),76,10,WizardryItemText::marked(rom_,name,(flags&2)!=0),focus_==side&&row_[side]==i});
            }
        }
        out.push_back({2,100,156,12,"未識別品を集める",row_[focus_]==8});out.push_back({2,112,156,12,"未装備品を相手へ移動",row_[focus_]==9});
        out.push_back({2,128,156,14,message_.empty()?(row_[focus_]<count(record(core,focus_))?itemName(record(core,focus_)[inventory+row_[focus_]*2],record(core,focus_)[inventory+row_[focus_]*2+1]):"B/N・L/R:キャラ  B:戻る"):message_});
        if(submenu_){
            const char* labels[]={"装備","移動","削除","上に移動","下に移動"};auto rec=record(core,focus_);
            for(unsigned i=0;i<5;i++)out.push_back({30,int(35+i*12),100,12,i==0&&(rec[inventory+row_[focus_]*2+1]&2)?"装備解除":labels[i],subRow_==i});
        }
        if(deleting_){out.push_back({20,50,120,16,"削除しますか？"});out.push_back({20,66,60,16,"はい",yes_});out.push_back({80,66,60,16,"いいえ",!yes_});}
        return out;
    }
};
