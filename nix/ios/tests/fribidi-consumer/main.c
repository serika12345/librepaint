#include <fribidi.h>

#include <stddef.h>

#if FRIBIDI_MAJOR_VERSION != 1 || FRIBIDI_MINOR_VERSION != 0 || FRIBIDI_MICRO_VERSION != 17
#  error "The Krita iOS FriBidi contract requires version 1.0.17"
#endif

#if FRIBIDI_INTERFACE_VERSION != 4
#  error "The Krita iOS FriBidi contract requires interface version 4"
#endif

#if FRIBIDI_SIZEOF_INT != 4
#  error "The Krita iOS FriBidi contract requires a four-byte int"
#endif

int main(void)
{
    static const FriBidiChar text[] = {'A', '(', 0x05D0, ')'};
    FriBidiCharType bidi_types[sizeof(text) / sizeof(text[0])];
    FriBidiBracketType bracket_types[sizeof(text) / sizeof(text[0])];
    FriBidiLevel embedding_levels[sizeof(text) / sizeof(text[0])];
    FriBidiParType base_direction = FRIBIDI_PAR_ON;
    const FriBidiStrIndex length = (FriBidiStrIndex)(sizeof(text) / sizeof(text[0]));

    fribidi_get_bidi_types(text, length, bidi_types);
    fribidi_get_bracket_types(text, length, bidi_types, bracket_types);
    const FriBidiLevel maximum_level = fribidi_get_par_embedding_levels_ex(
        bidi_types,
        bracket_types,
        length,
        &base_direction,
        embedding_levels
    );

    return maximum_level == 0 || base_direction == FRIBIDI_PAR_ON;
}
