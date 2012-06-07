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

package com.adobe.AGALOptimiser.agal {

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public final class RegisterCategorySet
{
    private var allowableCategories_ : Vector.<Boolean> = new Vector.<Boolean>(RegisterCategory.LAST_CATEGORY - RegisterCategory.FIRST_CATEGORY + 1, true);

    public function RegisterCategorySet(... categories)
    {
        var i : int;

        for (i = 0 ; i < allowableCategories_.length ; ++i)
            allowableCategories_[i] = false;

        for (i = 0 ; i < categories.length ; ++i)
        {
            if (categories[i] is RegisterCategory)
                allowableCategories_[(categories[i] as RegisterCategory).code - RegisterCategory.FIRST_CATEGORY] = true;
            else if (categories[i] is int)
                allowableCategories_[(categories[i] as int) - RegisterCategory.FIRST_CATEGORY] = true;
            else
                throw new Error(ErrorMessages.INVALID_VALUE);
        }
    }

    nsinternal function isAllowableCategory(rc : RegisterCategory) : Boolean
    {
        return allowableCategories_[rc.code - RegisterCategory.FIRST_CATEGORY];
    }

    nsinternal static function allCategories() : RegisterCategorySet
    {
        var ac : RegisterCategorySet = new RegisterCategorySet();

        for (var i : int = 0 ; i < ac.allowableCategories_.length ; ++i)
            ac.allowableCategories_[i] = true;

        return ac;
    }

} // RegisterCategorySet
} // com.adobe.AGALOptimiser.asm
