#include <ATEMmin.h>
#include <cstdio>
#include <string>
using Bytes = WiFiUDP::Bytes;
static unsigned failures = 0;
void check(bool result, const char* name) { printf("%s: %s\n", result ? "PASS" : "FAIL", name); failures += !result; }
Bytes command(const char* name, Bytes payload={}) {
  Bytes out(8); out[1] = 8+payload.size();
  std::copy_n(name,4,out.begin()+4); out.insert(out.end(),payload.begin(),payload.end()); return out;
}
Bytes packet(uint8_t flags, uint16_t id, Bytes payload={}, uint16_t sid=0x1234) {
  Bytes out(12); const uint16_t size = 12+payload.size();
  out[0]=(flags<<3)|(size>>8); out[1]=size; out[2]=sid>>8; out[3]=sid; out[10]=id>>8; out[11]=id;
  out.insert(out.end(),payload.begin(),payload.end()); return out;
}
void start(ATEMmin& atem) {
  WiFiUDP::incoming.clear(); WiFiUDP::outgoing.clear(); fakeMillis=100;
  atem.begin(IPAddress(192,168,1,196)); atem.runLoop();
  WiFiUDP::receive(packet(2,0,{2,0,0,0,0,0,0,0})); atem.runLoop();
}
int main() {
  {
    ATEMmin a{}; a.begin(IPAddress(192,168,1,196));
    const auto before=WiFiUDP::begins; a.connect(); a.runLoop();
    check(WiFiUDP::begins==before+1,"explicit connect does not send a second hello in runLoop");
  }
  {
    ATEMmin a{}; start(a);
    auto body=command("InCm"); auto tally=command("TlIn",{0,2,1,2}); body.insert(body.end(),tally.begin(),tally.end());
    WiFiUDP::receive(packet(1,1,body)); a.runLoop();
    check(a.getTallyByIndexSources()==2 && a.getTallyByIndexTallyFlags(0)==1,"zero-payload InCm does not discard following tally");
    WiFiUDP::receive(packet(1,2)); a.runLoop();
    check(a.hasInitialized(),"complete initial transfer becomes ready");
    a.connect(); check(a.getTallyByIndexSources()==0,"reconnect clears previous tally");
  }
  {
    ATEMmin a{}; start(a); WiFiUDP::outgoing.clear();
    WiFiUDP::receive(packet(1|4,1,command("TlIn",{0,2,1,2}))); a.runLoop();
    bool ack=false; for (const auto& p: WiFiUDP::outgoing) if((p[0]>>3)&16) ack=true;
    check(ack,"retransmitted packet receives acknowledgement");
    WiFiUDP::receive(packet(1,2,command("TlIn",{0,2,2,1}))); a.runLoop();
    WiFiUDP::receive(packet(1|4,1,command("TlIn",{0,2,1,2}))); a.runLoop();
    check(a.getTallyByIndexTallyFlags(0)==2,"old retransmit cannot overwrite newer tally");
  }
  {
    ATEMmin a{}; start(a);
    WiFiUDP::receive(packet(1,1,command("TlIn",{0,2,1,2}))); a.runLoop();
    WiFiUDP::receive(packet(1,2,command("TlIn",{0,8,2}))); a.runLoop();
    check(a.getTallyByIndexSources()==2,"truncated tally does not fabricate camera states");
  }
  {
    ATEMmin a{}; start(a);
    WiFiUDP::receive(packet(1,1,command("TlIn",{0,2,1,2}))); a.runLoop();
    WiFiUDP::receive(packet(1,2,command("TlIn",{0,2,2,1})),IPAddress(192,168,1,42)); a.runLoop();
    check(a.getTallyByIndexTallyFlags(0)==1,"unrelated sender cannot change tally");
    WiFiUDP::receive(Bytes{0,12,0x77}); a.runLoop();
    check(a.getSessionID()==0x1234,"short UDP header cannot change session");
  }
  {
    ATEMmin a{}; start(a); WiFiUDP::outgoing.clear();
    WiFiUDP::receive(packet(1,2,command("InCm"))); a.runLoop();
    check(!a.hasInitialized() && !a.streamHealthy(),"missing initial packet prevents ready");
    bool requested=false;
    for (const auto& p: WiFiUDP::outgoing)
      if (((p[0]>>3)&8) && word(p[6],p[7])==1) requested=true;
    check(requested,"resend request names the missing packet itself");
    WiFiUDP::receive(packet(5,1,command("TlIn",{0,2,1,2})));
    WiFiUDP::receive(packet(5,2,command("InCm"))); a.runLoop();
    check(a.hasInitialized() && a.streamHealthy() && a.getTallyByIndexSources()==2,
      "lost tally recovered before initialization completes");
    const auto sessions=a.diagnostics.sessions;
    for(int i=0;i<6;++i) { fakeMillis+=1000; WiFiUDP::receive(packet(1,5)); a.runLoop(); }
    check(a.diagnostics.sessions>sessions && a.getTallyByIndexSources()==0,
      "heartbeats ahead of an unrecoverable gap cannot keep a broken session alive");
  }
  {
    ATEMmin a{}; start(a);
    for(uint16_t id=1;id<32768;++id) { WiFiUDP::receive(packet(1,id)); a.runLoop(); }
    WiFiUDP::receive(packet(1,0,command("TlIn",{0,2,2,1}))); a.runLoop();
    check(a.getATEM_lastRemotePacketId()==0 && a.getTallyByIndexTallyFlags(0)==2,
      "packet sequence wraps from 32767 to zero without losing tally");
    fakeMillis=15;
    check(!a.hasTimedOut(0xfffffff0UL,32) && a.hasTimedOut(0xfffffff0UL,31),
      "timeout survives millis rollover");
  }
  {
    ATEMmin a{}; start(a);
    WiFiUDP::receive(packet(1,1,Bytes{0,100,0,0,'T','l','I','n',0,2,1,2})); a.runLoop();
    check(a.getTallyByIndexSources()==0 && a.diagnostics.commandErrors==1,
      "command exceeding datagram is rejected");
  }
  return failures ? 1 : 0;
}
