/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include "../src/input_processor_suppress.c"
static struct suppress_data data;
static const struct device dev={.data=&data,.api=&suppress_api};
static const uint32_t routes[]={SUPPRESS_ABS,SUPPRESS_ALL,SUPPRESS_TOUCH|SUPPRESS_ABS};
static struct input_event send(unsigned source,uint32_t mask,uint8_t type,uint16_t code,int32_t value,bool sync) {
    struct input_event e={.dev=&dev,.type=type,.code=code,.value=value,.sync=sync};
    struct zmk_input_processor_state state={.input_device_index=(uint8_t)source};
    assert(suppress_event(&dev,&e,mask,0,&state)==0); return e;
}
static void edge(unsigned source,uint32_t mask,uint16_t code,int32_t value,bool pass) {
    const struct input_event e=send(source,mask,INPUT_EV_KEY,code,value,false);
    assert((e.type==INPUT_EV_KEY)==pass && "independent button ownership");
    assert(e.sync==pass);
    if(pass) { assert(e.code==code && e.value==value); }
    else { assert(e.type==INPUT_EV_MSC && e.code==INPUT_MSC_SCAN && e.value==0); }
}
static void repeats(void) {
    const uint16_t codes[]={INPUT_BTN_TOUCH,INPUT_BTN_0};
    const int32_t values[]={1,2,INT32_MAX,-1}; unsigned count=0;
    for(unsigned source=0;source<2;source++) for(unsigned b=0;b<2;b++)
    for(unsigned route=0;route<3;route++) for(unsigned v=0;v<4;v++) {
        assert(suppress_init(&dev)==0); edge(source,SUPPRESS_ABS,codes[b],1,true);
        edge(source,routes[route],codes[b],values[v],v==3);
        edge(source,routes[route],codes[b],0,v!=3);
        assert(data.sources[source].forwarded==0); count++;
    }
    assert(count==48); puts("repeat/policy/routes: 48 cases PASS; negative means release");
}
static void overlaps(void) {
    const uint16_t codes[]={INPUT_BTN_TOUCH,INPUT_BTN_0}; unsigned count=0;
    for(unsigned source=0;source<2;source++) for(unsigned first=0;first<2;first++) {
        for(unsigned order=0;order<2;order++) {
            assert(suppress_init(&dev)==0);
            edge(source,0,codes[first],1,true); edge(source,SUPPRESS_ALL,codes[1-first],1,false);
            edge(source,SUPPRESS_ALL,codes[order ? first:1-first],0,order!=0);
            edge(source,SUPPRESS_ALL,codes[order ? 1-first:first],0,order==0); count++;
        }
        for(unsigned route=0;route<3;route++) {
            assert(suppress_init(&dev)==0); edge(source,0,codes[first],1,true); edge(source,0,codes[1-first],1,true);
#ifdef TEST_SHARED_HISTORY
            data.sources[source].forwarded=BIT(first==0 ? 16:0);
#endif
            edge(source,routes[route],codes[first],2,false); edge(source,routes[route],codes[first],0,true);
            edge(source,routes[route],codes[1-first],0,true); count++;
        }
    }
    for(unsigned source=0;source<2;source++) for(unsigned b=0;b<2;b++) {
        assert(suppress_init(&dev)==0); edge(source,0,codes[b],1,true); edge(1-source,SUPPRESS_ALL,codes[b],1,false);
        edge(1-source,0,codes[b],0,false); edge(source,SUPPRESS_ALL,codes[b],0,true); count++;
    }
    assert(count==24); puts("overlapping buttons/listeners: 24 cases PASS");
}
static void frames_and_lifetime(void) {
    for(unsigned slot=0;slot<17;slot++) {
        assert(suppress_init(&dev)==0);
        uint16_t code=slot==16 ? INPUT_BTN_TOUCH:(uint16_t)(INPUT_BTN_0+slot);
        edge(0,SUPPRESS_ALL,code,1,false); edge(0,0,code,2,false); edge(0,0,code,0,false);
        if(slot==16) { /* A distinct contact clears its old suppression mask. */
            assert(!data.sources[0].touching);
        }
        edge(0,0,code,1,true); edge(0,SUPPRESS_ALL,code,0,true); edge(0,0,code,0,false);
    }
    assert(suppress_init(&dev)==0);
    edge(0,0,42,1,false); edge(0,0,INPUT_BTN_0+16,1,false);
    struct input_event e=send(0,0,INPUT_EV_MSC,7,9,false);
    assert(e.type==INPUT_EV_MSC && e.code==7 && e.value==9 && !e.sync);
    e=send(0,0,INPUT_EV_REL,0,7,false); assert(e.value==7 && !e.sync);
    e=send(1,SUPPRESS_ALL,INPUT_EV_ABS,0,100,true); assert(e.type==INPUT_EV_MSC && !e.sync);
    e=send(0,SUPPRESS_ALL,INPUT_EV_ABS,0,100,true); assert(e.type==INPUT_EV_MSC && e.sync);
    e=send(0,SUPPRESS_ALL,INPUT_EV_ABS,0,100,true); assert(!e.sync);
    e=send(0,0,INPUT_EV_MSC,INPUT_MSC_SCAN,0,true); assert(!e.sync);
    e=send(0,0,INPUT_EV_ABS,0,100,true); assert(e.type==INPUT_EV_ABS && e.value==100 && e.sync);
    e=send(0,0,INPUT_EV_ABS,1,99,false); assert(e.type==INPUT_EV_ABS && !e.sync);
    e=send(0,0,INPUT_EV_MSC,INPUT_MSC_SCAN,0,true); assert(e.type==INPUT_EV_MSC && e.sync);
    puts("all17 slots, unmatched/unknown edges, source-local pending/silent frames and ABS PASS");
}
static void invalid(void) {
    assert(suppress_init(&dev)==0);
    struct zmk_input_processor_state state={.input_device_index=2};
    struct input_event e={.type=INPUT_EV_KEY,.code=INPUT_BTN_0,.value=1,.sync=true};
    assert(suppress_event(&dev,&e,0,0,&state)==ZMK_INPUT_PROC_STOP);
    assert(e.type==INPUT_EV_MSC && !e.sync && data.sources[0].forwarded==0);
    state.input_device_index=0;
    e=(struct input_event){.type=INPUT_EV_KEY,.code=INPUT_BTN_0,.value=1,.sync=true};
    assert(suppress_event(&dev,&e,UINT32_MAX,0,&state)==ZMK_INPUT_PROC_STOP);
    assert(e.type==INPUT_EV_MSC && !e.sync && data.sources[0].forwarded==0);
    e=(struct input_event){.type=INPUT_EV_REL,.value=7,.sync=true};
    assert(suppress_event(&dev,&e,0,0,NULL)==0 && e.value==7);
    edge(0,0,INPUT_BTN_0,1,true);
    assert(suppress_init(&dev)==0 && data.sources[0].forwarded==0);
    puts("invalid source/mask fail closed without history alias; legacy NULL state and cold init PASS");
}
static void contact_policy(void) {
    const uint32_t pointer=SUPPRESS_TOUCH|SUPPRESS_ABS;
    const uint32_t scroll=pointer|SUPPRESS_BTN0;
    for(unsigned source=0;source<2;source++) {
        assert(suppress_init(&dev)==0);
        edge(source,pointer,INPUT_BTN_TOUCH,1,false);
        /* A route may change on a coordinate, repeat, or even the touch-up. */
        send(source,scroll,INPUT_EV_REL,0,4,true);
        edge(source,scroll,INPUT_BTN_TOUCH,2,false);
        edge(source,pointer,INPUT_BTN_TOUCH,-1,false);
#ifdef TEST_NO_CONTACT_LATCH
        data.sources[source].contact_mask=0;
#endif
        const struct input_event e=send(source,pointer,INPUT_EV_KEY,INPUT_BTN_0,1,true);
        assert(e.type==INPUT_EV_MSC && "delayed click remains suppressed after route exit");
        edge(source,pointer,INPUT_BTN_0,0,false);
        edge(source,pointer,INPUT_BTN_TOUCH,1,false);
        edge(source,pointer,INPUT_BTN_0,1,true);
        edge(source,scroll,INPUT_BTN_TOUCH,2,false);
        edge(source,pointer,INPUT_BTN_0,0,true); /* Owed release survives stricter contact policy. */
        edge(source,pointer,INPUT_BTN_0,1,false); edge(source,pointer,INPUT_BTN_0,0,false);
        edge(source,pointer,INPUT_BTN_TOUCH,0,false);
    }
    assert(suppress_init(&dev)==0);
    edge(0,pointer,INPUT_BTN_TOUCH,1,false); edge(1,pointer,INPUT_BTN_TOUCH,1,false);
    edge(0,scroll,INPUT_BTN_TOUCH,0,false); /* The lifting event itself contributes. */
    edge(1,pointer,INPUT_BTN_TOUCH,0,false);
    edge(0,pointer,INPUT_BTN_0,1,false); edge(0,pointer,INPUT_BTN_0,0,false);
    edge(1,pointer,INPUT_BTN_0,1,true); edge(1,pointer,INPUT_BTN_0,0,true);
    edge(1,pointer,INPUT_BTN_TOUCH,1,false); edge(1,scroll,INPUT_BTN_TOUCH,0,false);
    edge(0,pointer,INPUT_BTN_TOUCH,1,false); /* Clears source0 only. */
    edge(0,pointer,INPUT_BTN_0,1,true); edge(0,pointer,INPUT_BTN_0,0,true);
    edge(1,pointer,INPUT_BTN_0,1,false); edge(1,pointer,INPUT_BTN_0,0,false);
    /* A fresh touch does not clear a still-owed or suppressed physical pair. */
    edge(0,pointer,INPUT_BTN_0,1,true); edge(0,pointer,INPUT_BTN_TOUCH,0,false);
    edge(0,scroll,INPUT_BTN_TOUCH,1,false); edge(0,scroll,INPUT_BTN_0,0,true);
    edge(0,scroll,INPUT_BTN_0,1,false); edge(0,scroll,INPUT_BTN_TOUCH,0,false);
    edge(0,pointer,INPUT_BTN_TOUCH,1,false); edge(0,pointer,INPUT_BTN_0,2,false);
    edge(0,pointer,INPUT_BTN_0,0,false);
    edge(0,pointer,INPUT_BTN_0,1,true); edge(0,pointer,INPUT_BTN_0,0,true);
    /* ABS suppression is local to the current route, not latched as a button. */
    send(0,SUPPRESS_ABS,INPUT_EV_ABS,0,1,false);
    const struct input_event abs=send(0,0,INPUT_EV_ABS,0,2,true); assert(abs.type==INPUT_EV_ABS && abs.value==2);
    assert(suppress_init(&dev)==0);
    /* Pure click sources get pair disposition but no invented contact latch. */
    edge(0,SUPPRESS_BTN0,INPUT_BTN_0,1,false); edge(0,0,INPUT_BTN_0,0,false);
    edge(0,0,INPUT_BTN_0,1,true); edge(0,0,INPUT_BTN_0,0,true);
    puts("contact masks: traversed routes, delayed tap, repeats, source independence, owed pairs and recovery PASS");
}
int main(void) { repeats(); overlaps(); frames_and_lifetime(); invalid(); contact_policy(); puts("final suppression driver: PASS"); return 0; }
