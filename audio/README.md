# Audio

Reports built with this framework can strip out code from the plaintext, and
allow for tweaking the stream-processing of plaintext through a text-to-speech
backend.

There are several text to speech engines available:

| festival | build process is difficult, segmentation faults often reported |
| espeak   | pretty good overall, but very robotic sounding |
| google translate | the most human sounding by far. Default |

There are also commercial options for text to speech, such as google cloud's
translation services and direct apis.

To perform the translation, `gtts` is used, which is a wrapper around google
translate.

## Phonetic corrections

To override how the engine should speak, a 2-tuple of phonetic corrections can
be read from `audio/phonetics.txt`.

An example:

```
mctop "em see top"
uprobe "you probe"
kubectl "kube see tee el"
```

You can also use `...` to force longer pauses and control emphasis. To tweak
the translation to match how something should naturally be spoken (at least, to
the author), add corrections to this file.

# Building audio

To build the audio, the plaintext version of the document is rendered. Several
regex filters are used to apply corrections, cutting and pasting in text to
be translated.

Each document in the docs folder will have a plaintext transcript generated,
and then this is piped into the speech engine and recorded.

# Embedding audio

To use these `mp3` files with outputs formats that support it, a tag is
inserted at the beginning of each level one header. This is what generates
the pause/play dialog in modern web browsers and other compatible content
players.

## Why generate audio?

For proofreading purposes, hearing text read-back aloud is really helpful in
catching typos and confusing wording.

Saving this output and embedding it in the output, the documents become
accessible to those who cannot read them as easily.
