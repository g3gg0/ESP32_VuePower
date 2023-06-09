
#include <MD_RTTTLParser.h>
#include <MD_MusicTable.h>

MD_RTTTLParser Tone;
MD_MusicTable Table;

void rtttl_cb (uint8_t octave, uint8_t noteId, uint32_t duration, bool activate)
{
    if(activate)
    {
        if(Table.findNoteOctave(noteId, octave))
        {
            buzz_on(Table.getFrequency());
        }
    }
    else
    {
        buzz_off();
    }
}

void rtttl_setup()
{
    Tone.begin();
    Tone.setCallback(&rtttl_cb);
}

bool rtttl_loop()
{
    Tone.run();
    
    return false;
}

void rtttl_play(const char *rtttl)
{
    Tone.setTune(rtttl);
}
