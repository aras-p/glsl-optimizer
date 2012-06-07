/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

package com.adobe.AGALOptimiser.utils {

import com.adobe.AGALOptimiser.nsinternal;

[ExcludeClass]
public final class Tokenizer
{
    private var tokens_          : Vector.<String> = null;
    private var currentLocation_ : int             = 0;
    private var stream_          : String          = null;
    private var delimiters_      : RegExp          = undefined;

    public function Tokenizer(delimiters : RegExp)
    {
        delimiters_ = delimiters;

    }

    internal function getNextToken() : String
    {
        var whitespace : RegExp = /\s/;

        while ((currentLocation_ < stream_.length) && whitespace.test(stream_.charAt(currentLocation_)) )
            ++currentLocation_;

        if (currentLocation_ >= stream_.length)
            return null;

        // otherwise, we know we have something useful.  currentLocation_ is pointing at the char
        // which is either punctuation or a meaningful token.  If it's punctuation, we
        // return that and move the location forward, otherwise, we accumulate until whitespace
        // or punctuataion

        if (delimiters_.test(stream_.charAt(currentLocation_)) )
        {
            return stream_.charAt(currentLocation_++);
        }
        else if (stream_.charAt(currentLocation_) == "\"" )
        {
            return getStringToken();
        }
        else
        {
            var token : String = "";

            while ((currentLocation_ < stream_.length) &&
                   (!whitespace.test(stream_.charAt(currentLocation_))) &&
                   (!delimiters_.test(stream_.charAt(currentLocation_))))
            {
                token += stream_.charAt(currentLocation_);
                if(token == "!!")
                {
                    while(stream_.charAt(currentLocation_) !="\n")
                        ++currentLocation_;
                    token = getNextToken();
                    break;
                }
                ++currentLocation_;
            }

            return token;
        }
    }

    internal function getStringToken() : String
    {
        // Skip the opening double quote
        ++currentLocation_;

        var result : String = "";

        // TBD: Handle embedded quote characters
        while ( currentLocation_ < stream_.length && stream_.charAt( currentLocation_ ) != "\"" )
        {
            result += stream_.charAt( currentLocation_++ );
        }

        // Skip the closing double quote
        ++currentLocation_;

        return result;
    }

    nsinternal function tokenize(stream : String) : Vector.<String>
    {
        var finalResult : Vector.<String> = null;

        try
        {
            tokens_          = new Vector.<String>();
            stream_          = stream;
            currentLocation_ = 0;

            var nextToken : String = null;

            while (nextToken = getNextToken()) {
                tokens_.push(nextToken);
            }

            finalResult = tokens_;
        }
        finally
        {
            tokens_          = null;
            currentLocation_ = 0;
            stream_          = null;
        }

        return finalResult;
    }

} // Tokenizer
} // com.adobe.AGALOptimiser.utils
