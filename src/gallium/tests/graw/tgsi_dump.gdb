define tgsi_dump
    set $tokens=(const struct tgsi_header *)($arg0)
    set $nr_tokens = $tokens->HeaderSize + $tokens->BodySize
    set $tokens_end = &$tokens[$nr_tokens]
    dump memory tgsi_dump.bin $tokens $tokens_end
end
