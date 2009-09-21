
# apiutil.py
#
# This file defines a bunch of utility functions for OpenGL API code
# generation.

import sys, string, re


#======================================================================

def CopyrightC( ):
        print """/* Copyright (c) 2001, Stanford University
        All rights reserved.

        See the file LICENSE.txt for information on redistributing this software. */
        """

def CopyrightDef( ):
        print """; Copyright (c) 2001, Stanford University
        ; All rights reserved.
        ;
        ; See the file LICENSE.txt for information on redistributing this software.
        """


#======================================================================

class APIFunction:
        """Class to represent a GL API function (name, return type,
        parameters, etc)."""
        def __init__(self):
                self.name = ''
                self.returnType = ''
                self.category = ''
                self.categories = []
                self.offset = -1
                self.alias = ''
                self.vectoralias = ''
                self.convertalias = ''
                self.aliasprefix = ''
                self.params = []
                self.paramlist = []
                self.paramvec = []
                self.paramaction = []
                self.paramprop = []
                self.paramset = []
                self.props = []
                self.chromium = []

def FindParamIndex(params, paramName):
        """Given a function record, find the index of a named parameter"""
        for i in range (len(params)):
                if paramName == params[i][0]:
                        return i
        # If we get here, there was no such parameter
        return None

def SetTupleIndex(tuple, index, value):
        t = ()
        for i in range(len(tuple)):
                if i == index:
                        t += (value,)
                else:
                        t += (tuple[i],)
        return t

def VersionSpecificValues(category, values):
        selectedValues = []
        for value in values:
                # Version-specific values are prefixed with the version
                # number, e.g. GLES1.0:GL_TEXTURE_CUBE_MAP_OES
                splitValue = value.split(":")
                if len(splitValue) == 2:
                        if category != None and splitValue[0] != category:
                                # Don't want this one.
                                continue
                        else:
                                selectedValues.append(splitValue[1])
                else:
                        selectedValues.append(value)
        return selectedValues

def ProcessSpecFile(filename, userFunc, category = None):
        """Open the named API spec file and call userFunc(record, category) for each record
        processed."""
        specFile = open(filename, "r")
        if not specFile:
                print >>sys.stderr, "Error: couldn't open %s file!" % filename
                sys.exit()

        record = APIFunction()

        for line in specFile.readlines():

                # split line into tokens
                tokens = string.split(line)

                if len(tokens) > 0 and line[0] != '#':

                        if tokens[0] == 'name':
                                if record.name != '':
                                        # process the old function now
                                        userFunc(record, category)
                                        # reset the record
                                        record = APIFunction()

                                record.name = tokens[1]

                        elif tokens[0] == 'return':
                                record.returnType = string.join(tokens[1:], ' ')
                        
                        elif tokens[0] == 'param':
                                name = tokens[1]
                                type = string.join(tokens[2:], ' ')
                                vecSize = 0
                                record.params.append((name, type, vecSize, None, [], None))

                        elif tokens[0] == 'paramprop':
                                name = tokens[1]
                                str = tokens[2:]
                                enums = []
                                for i in range(len(str)):
                                        enums.append(str[i]) 
                                record.paramprop.append((name, enums))

                        elif tokens[0] == 'paramlist':
                                name = tokens[1]
                                str = tokens[2:]
                                list = []
                                for i in range(len(str)):
                                        list.append(str[i]) 
                                record.paramlist.append((name,list))

                        elif tokens[0] == 'paramvec':
                                name = tokens[1]
                                str = tokens[2:]
                                vec = []
                                for i in range(len(str)):
                                        vec.append(str[i]) 
                                record.paramvec.append((name,vec))

                        elif tokens[0] == 'paramset':
                                line = tokens[1:]
                                result = []
                                for i in range(len(line)):
                                        tset = line[i]
                                        if tset == '[':
                                                nlist = []
                                        elif tset == ']':
                                                result.append(nlist)
                                                nlist = []
                                        else:
                                                nlist.append(tset)
                                if result != []:
                                        record.paramset.append(result)

                        elif tokens[0] == 'paramaction':
                                name = tokens[1]
                                str = tokens[2:]
                                list = []
                                for i in range(len(str)):
                                        list.append(str[i]) 
                                record.paramaction.append((name,list))

                        elif tokens[0] == 'category':
                                record.category = tokens[1]
                                record.categories = tokens[1:]

                        elif tokens[0] == 'offset':
                                if tokens[1] == '?':
                                        record.offset = -2
                                else:
                                        record.offset = int(tokens[1])

                        elif tokens[0] == 'alias':
                                record.alias = tokens[1]

                        elif tokens[0] == 'vectoralias':
                                record.vectoralias = tokens[1]

                        elif tokens[0] == 'convertalias':
                                record.convertalias = tokens[1]

                        elif tokens[0] == 'aliasprefix':
                                record.aliasprefix = tokens[1]

                        elif tokens[0] == 'props':
                                record.props = tokens[1:]

                        elif tokens[0] == 'chromium':
                                record.chromium = tokens[1:]

                        elif tokens[0] == 'vector':
                                vecName = tokens[1]
                                vecSize = int(tokens[2])
                                index = FindParamIndex(record.params, vecName)
                                if index == None:
                                        print >>sys.stderr, "Can't find vector '%s' for function '%s'" % (vecName, record.name)
                                # Adjust just the vector size
                                record.params[index] = SetTupleIndex(record.params[index], 2, vecSize)

                        elif tokens[0] == 'dependentvector':
                                dependentVecName = tokens[1]
                                # the dependentVecSize may be an int
                                # expression
                                dependentVecSize = tokens[2]
                                controllingParam = tokens[3]
                                controllingParamIndex = FindParamIndex(record.params, controllingParam)
                                if controllingParamIndex == None:
                                        print >>sys.stderr, "Can't find controlling param '%s' for function '%s'" % (controllingParam, record.name)
                                controllingValues = tokens[4:]

                                # Remember that all of the controllingValues
                                # are valid values for the controllingParam.
                                # We may be duplicating controllingValues
                                # here (i.e. if we get them from different
                                # places); we'll sort them out later.
                                validValues = record.params[controllingParamIndex][4]
                                for value in VersionSpecificValues(category, controllingValues):
                                        validValues.append((value, dependentVecSize, dependentVecName, [], None, None))
                                # Don't need to reassign validValues back
                                # to the tuple - it's a shallow pointer,
                                # so the tuple is already modified.  
                                # (And attempting to do so would produce an
                                # error anyway.)

                        elif tokens[0] == "dependentnovalueconvert":
                                paramName = tokens[1]
                                controllingParamName = tokens[2]
                                controllingValues = tokens[3:]

                                controllingParamIndex = FindParamIndex(record.params, controllingParamName)
                                if controllingParamIndex == None:
                                        print >>sys.stderr, "Can't find controlling param '%s' for function '%s'" % (controllingParamName, record.name)

                                validValues = record.params[controllingParamIndex][4]
                                for value in VersionSpecificValues(category, controllingValues):
                                        validValues.append((value, None, None, [], None, "noconvert"))



                        elif tokens[0] == 'checkparam':
                                paramName = tokens[1]
                                values = tokens[2:]
                                paramIndex = FindParamIndex(record.params, paramName)
                                if paramIndex == None:
                                        print >>sys.stderr, "Can't find checked param '%s' for function '%s'" % (paramName, record.name)

                                errorCode = None

                                # We may be duplicating valid values here;
                                # just add all values to the existing
                                # record, and we'll prune out duplicates
                                # later
                                validValues = record.params[paramIndex][4]

                                # A /GL_* value represents an error, not
                                # a real value.  Look through the values
                                # and only append the non-error values.
                                for v in VersionSpecificValues(category, values):
                                        if v[0] == "/":
                                                errorCode = v[1:]
                                        else:
                                            validValues.append((v, None, None, [], errorCode, None))
                                # Don't need to reassign validValues back
                                # to the parameter tuple - it's a shallow pointer,
                                # so the tuple is already modified.  
                                # (And attempting to do so would produce an
                                # error anyway.)

                        elif tokens[0] == 'checkdependentparam':
                                paramName = tokens[1]
                                controllingValue = tokens[2]
                                dependentParamName = tokens[3]
                                validDependentValues = tokens[4:]
                                errorCode = None

                                # A /GL_* value represents an error, not
                                # a real value.  Look through the values
                                # and only append the non-error values.
                                validDependentValues = []
                                for v in tokens[4:]:
                                        if v[0] == "/":
                                                errorCode = v[1:]
                                        else:
                                            validDependentValues.append(v)

                                paramIndex = FindParamIndex(record.params, paramName)
                                if paramIndex == None:
                                        print >>sys.stderr, "Can't find dependent param '%s' for function '%s'" % (paramName, record.name)

                                validValues = record.params[paramIndex][4]
                                # We may be duplicating valid values here;
                                # we'll sort them out later.  Avoid
                                # adding a controlling value record
                                # at all if there are no values
                                # in the list of values (so that
                                # controlling values with only
                                # version-specific values listed
                                # end up as version-specific
                                # themselves).
                                versionSpecificValues = VersionSpecificValues(category, validDependentValues)
                                if versionSpecificValues != []:
                                        validValues.append((controllingValue, None, dependentParamName, versionSpecificValues, errorCode, None))
                                # Don't need to reassign validValues back
                                # to the tuple - it's a shallow pointer,
                                # so the tuple is already modified.  
                                # (And attempting to do so would produce an
                                # error anyway.)

                        elif tokens[0] == 'convertparams':
                                convertToType = tokens[1]
                                # Replace the conversion type in each named
                                # parameter
                                for paramName in tokens[2:]:
                                        paramIndex = FindParamIndex(record.params, paramName)
                                        if paramIndex == None:
                                            print >>sys.stderr, "Can't find converted param '%s' for function '%s'" % (paramName, record.name)
                                        # Tuples don't support item assignment,
                                        # so to replace scalar values in a
                                        # tuple, you have to reassign the
                                        # whole friggin' thing.
                                        record.params[paramIndex] = SetTupleIndex(record.params[paramIndex], 3, convertToType)

                        else:
                                print >>sys.stderr, 'Invalid token %s after function %s' % (tokens[0], record.name)
                        #endif
                #endif
        #endfor

        # Call the user function for the last record, if we still have one
        # lying around nearly finished
        if record.name != '':
                # process the function now
                userFunc(record, category)
        specFile.close()
#enddef


# Dictionary [name] of APIFunction:
__FunctionDict = {}

# Dictionary [name] of name
__VectorVersion = {}

# Reverse mapping of function name aliases
__ReverseAliases = {}

def CheckCategories(category, categories):
    for c in categories:
        if category == c.split(":")[0]:
            return 1

    return 0

def AddFunction(record, category):
        # If there is a category, we only want records from that category.
        # Note that a category may be in the form "GLES1.1:OES_extension_name",
        # which means that the function is supported as an extension in 
        # GLES1.1...
        if category and not CheckCategories(category, record.categories):
                return

        # Don't allow duplicates
        if __FunctionDict.has_key(record.name):
                print >>sys.stderr, "Duplicate record name '%s' ignored" % record.name
                return

        # Clean up a bit.  We collected valid values for parameters
        # on the fly; it's quite possible that there are duplicates.
        # If there are, collect them together.
        # 
        # We're also going to keep track of all the dependent values
        # that can show up for a parameter; the number of GLenum values
        # (identified with a prefixed "GL_") affects how parameter
        # value conversion happens, as GLenum values are not scaled
        # when converted to or from GLfixed values, the way integer
        # and floating point values are.
        paramValueConversion = {}

        for i in range(len(record.params)):
                foundValidValues = {}

                (name, type, maxVecSize, convertToType, validValues, valueConversion) = record.params[i]
                for (controllingValue, vecSize, dependentParamName, dependentValues, errorCode, valueConvert) in validValues:
                        # Keep track of the maximum vector size for the
                        # *dependent* parameter, not for the controlling
                        # parameter.  Note that the dependent parameter
                        # may be an expression - in this case, don't
                        # consider it.
                        if dependentParamName != None and vecSize != None and vecSize.isdigit():
                                dependentParamIndex = FindParamIndex(record.params, dependentParamName)
                                if dependentParamIndex == None:
                                        print >>sys.stderr, "Couldn't find dependent parameter '%s' of function '%s'" % (dependentParamName, record.name)
                                (dName, dType, dMaxVecSize, dConvert, dValid, dValueConversion) = record.params[dependentParamIndex]
                                if dMaxVecSize == None or int(vecSize) > dMaxVecSize:
                                        dMaxVecSize = int(vecSize)
                                        record.params[dependentParamIndex] = (dName, dType, dMaxVecSize, dConvert, dValid, dValueConversion)

                        # Make sure an entry for the controlling value
                        # exists in the foundValidValues dictionary
                        if controllingValue in foundValidValues:
                                # The value was already there.  Merge the
                                # two together, giving errors if needed.
                                (oldControllingValue, oldVecSize, oldDependentParamName, oldDependentValues, oldErrorCode, oldValueConvert) = foundValidValues[controllingValue]

                                # Make sure the vector sizes are compatible;
                                # either one should be None (and be 
                                # overridden by the other), or they should
                                # match exactly.  If one is not an
                                # integer (this can happen if the 
                                # dependent value is an integer expression,
                                # which occurs a couple of times), don't
                                # use it.
                                if oldVecSize == None:
                                        oldVecSize = vecSize
                                elif vecSize != None and vecSize != oldVecSize:
                                        print >>sys.stderr, "Found two different vector sizes (%s and %s) for the same controlling value '%s' of the same parameter '%s' of function '%s'" % (oldVecSize, vecSize, controllingValue, name, record.name)

                                # Same for the dependent parameter name.
                                if oldDependentParamName == None:
                                        oldDependentParamName = dependentParamName
                                elif dependentParamName != None and dependentParamName != oldDependentParamName:
                                        print >>sys.stderr, "Found two different dependent parameter names (%s and %s) for the same controlling value '%s' of the same parameter '%s' of function '%s'" % (oldDependentParamName, dependentParamName, controllingValue, name, record.name)

                                # And for the error code.
                                if oldErrorCode == None:
                                        oldErrorCode = errorCode
                                elif errorCode != None and errorCode != oldErrorCode:
                                        print >>sys.stderr, "Found two different error codes(%s and %s) for the same controlling value '%s' of the same parameter '%s' of function '%s'" % (oldErrorCode, errorCode, controllingValue, name, record.name)

                                # And for the value conversion flag
                                if oldValueConvert == None:
                                        oldValueConvert = valueConvert
                                elif valueConvert != None and valueConvert != oldValueConvert:
                                        print >>sys.stderr, "Found two different value conversions(%s and %s) for the same controlling value '%s' of the same parameter '%s' of function '%s'" % (oldValueConvert, valueConvert, controllingValue, name, record.name)

                                # Combine the dependentValues together
                                # directly, but uniquely
                                for value in dependentValues:
                                        if value not in oldDependentValues:
                                                oldDependentValues.append(value)

                                # Stick the combined value back into the
                                # dictionary.  We'll sort it back to the
                                # array later.
                                foundValidValues[oldControllingValue] = (oldControllingValue, oldVecSize, oldDependentParamName, oldDependentValues, oldErrorCode, oldValueConvert)
                        else: # new controlling value
                                # Just add it to the dictionary so we don't
                                # add the same value more than once.
                                foundValidValues[controllingValue] = (controllingValue, vecSize, dependentParamName, dependentValues, errorCode, valueConvert)
                        # endif a new controlling value

                # endfor all valid values for this parameter

                # Now the foundValidValues[] dictionary holds all the
                # pruned values (at most one for each valid value).
                # But the validValues[] array still holds the order,
                # which we want to maintain.  Go through the validValues
                # array just for the names of the controlling values; 
                # add any uncopied values to the prunedValidValues array.
                prunedValidValues = []

                for (controllingValue, vecSize, dependentParamName, dependentValues, errorCode, valueConvert) in validValues:
                        if controllingValue in foundValidValues:
                                prunedValidValues.append(foundValidValues[controllingValue])
                                # Delete it from the dictionary so it isn't
                                # copied again.
                                del foundValidValues[controllingValue]

                # Each parameter that is being converted may have a
                # subset of values that are GLenums and are never
                # converted.  In some cases, the parameter will 
                # be implicitly determined to be such, by examining
                # the listed possible values and determining for
                # any particular controlling value whether there are
                # any GLenum-valued allowed values.
                #
                # In other cases, the parameter will be explicitly
                # marked as such with the "dependentnovalueconvert" flag;
                # this is the way it has to be done with queries (which
                # cannot list a valid list of values to be passed,
                # because values are returned, not passed).
                #
                # For each value of controlling GLenum, we'll also
                # need to know whether its dependent values are
                # always converted (none of its values require
                # GLenums to be passed in dependent parameters),
                # never converted (all of its values require GLenums),
                # or sometimes converted.
                numNoConvertValues = 0
                allDependentParams = []
                for j in range(len(prunedValidValues)):
                        (controllingValue, vecSize, dependentParamName, dependentValues, errorCode, valueConvert) = prunedValidValues[j]
                        if dependentParamName != None and dependentParamName not in allDependentParams:
                                allDependentParams.append(dependentParamName)

                        # Check for an explicit noconvert marking...
                        if valueConvert == "noconvert":
                                numNoConvertValues += 1
                        else:
                                # Or check for an implicit one.
                                for value in dependentValues:
                                        if value[0:3] == "GL_":
                                                valueConvert = "noconvert"
                                                prunedValidValues[j] = (controllingValue, vecSize, dependentParamName, dependentValues, errorCode, valueConvert)
                                                numNoConvertValues += 1
                                                break

                # For each named dependent param, set the value conversion
                # flag based on whether all values, none, or some need
                # value conversion.  This value is set stepwise
                # for each parameter examined - 
                for dp in allDependentParams:
                        if numNoConvertValues == 0:
                                if not paramValueConversion.has_key(dp):
                                        paramValueConversion[dp] = "all"
                                elif paramValueConversion[dp] == "none":
                                        paramValueConversion[dp] = "some"
                        elif numNoConvertValues == len(prunedValidValues):
                                if not paramValueConversion.has_key(dp):
                                        paramValueConversion[dp] = "none"
                                elif paramValueConversion[dp] == "all":
                                        paramValueConversion[dp] = "some"
                        else:
                                paramValueConversion[dp] = "some"

                # Save away the record.  Save a placeholder in the
                # valueConversion field - we can't set that until we've
                # examined all the parameters.
                record.params[i] = (name, type, maxVecSize, convertToType, prunedValidValues, None)

        # endfor each param of the passed-in function record

        # One more pass: for each parameter, if it is a parameter that
        # needs conversion, save its value conversion type ("all", "none",
        # or "some").  We only have to worry about conditional value 
        # conversion for GLfixed parameters; in all other cases, we
        # either don't convert at all (for non-converting parameters) or
        # we convert everything.
        for i in range(len(record.params)):
                (name, type, maxVecSize, convertToType, validValues, valueConversion) = record.params[i]
                if convertToType == None:
                    valueConversion = None
                elif paramValueConversion.has_key(name):
                    valueConversion = paramValueConversion[name]
                else:
                    valueConversion = "all"

                record.params[i] = (name, type, maxVecSize, convertToType, validValues, valueConversion)
                                        
        # We're done cleaning up!
        # Add the function to the permanent record and we're done.
        __FunctionDict[record.name] = record


def GetFunctionDict(specFile = "", category = None):
        if not specFile:
                specFile = "../glapi_parser/APIspec.txt"
        if len(__FunctionDict) == 0:
                ProcessSpecFile(specFile, AddFunction, category)
                # Look for vector aliased functions
                for func in __FunctionDict.keys():
                        va = __FunctionDict[func].vectoralias
                        if va != '':
                                __VectorVersion[va] = func
                        #endif

                        # and look for regular aliases (for glloader)
                        a = __FunctionDict[func].alias
                        if a:
                                __ReverseAliases[a] = func
                        #endif
                #endfor
        #endif
        return __FunctionDict


def GetAllFunctions(specFile = "", category = None):
        """Return sorted list of all functions known to Chromium."""
        d = GetFunctionDict(specFile, category)
        funcs = []
        for func in d.keys():
                rec = d[func]
                if not "omit" in rec.chromium:
                        funcs.append(func)
        funcs.sort()
        return funcs
        

def GetDispatchedFunctions(specFile = "", category = None):
        """Return sorted list of all functions handled by SPU dispatch table."""
        d = GetFunctionDict(specFile, category)
        funcs = []
        for func in d.keys():
                rec = d[func]
                if (not "omit" in rec.chromium and
                        not "stub" in rec.chromium and
                        rec.alias == ''):
                        funcs.append(func)
        funcs.sort()
        return funcs

#======================================================================

def ReturnType(funcName):
        """Return the C return type of named function.
        Examples: "void" or "const GLubyte *". """
        d = GetFunctionDict()
        return d[funcName].returnType


def Parameters(funcName):
        """Return list of tuples (name, type, vecSize) of function parameters.
        Example: if funcName=="ClipPlane" return
        [ ("plane", "GLenum", 0), ("equation", "const GLdouble *", 4) ] """
        d = GetFunctionDict()
        return d[funcName].params

def ParamAction(funcName):
        """Return list of names of actions for testing.
        For PackerTest only."""
        d = GetFunctionDict()
        return d[funcName].paramaction

def ParamList(funcName):
        """Return list of tuples (name, list of values) of function parameters.
        For PackerTest only."""
        d = GetFunctionDict()
        return d[funcName].paramlist

def ParamVec(funcName):
        """Return list of tuples (name, vector of values) of function parameters.
        For PackerTest only."""
        d = GetFunctionDict()
        return d[funcName].paramvec

def ParamSet(funcName):
        """Return list of tuples (name, list of values) of function parameters.
        For PackerTest only."""
        d = GetFunctionDict()
        return d[funcName].paramset


def Properties(funcName):
        """Return list of properties of the named GL function."""
        d = GetFunctionDict()
        return d[funcName].props

def AllWithProperty(property):
        """Return list of functions that have the named property."""
        funcs = []
        for funcName in GetDispatchedFunctions():
                if property in Properties(funcName):
                        funcs.append(funcName)
        return funcs

def Category(funcName):
        """Return the primary category of the named GL function."""
        d = GetFunctionDict()
        return d[funcName].category

def Categories(funcName):
        """Return all the categories of the named GL function."""
        d = GetFunctionDict()
        return d[funcName].categories

def ChromiumProps(funcName):
        """Return list of Chromium-specific properties of the named GL function."""
        d = GetFunctionDict()
        return d[funcName].chromium

def ParamProps(funcName):
        """Return list of Parameter-specific properties of the named GL function."""
        d = GetFunctionDict()
        return d[funcName].paramprop

def Alias(funcName):
        """Return the function that the named function is an alias of.
        Ex: Alias('DrawArraysEXT') = 'DrawArrays'.
        """
        d = GetFunctionDict()
        return d[funcName].alias

def AliasPrefix(funcName):
        """Return the function that the named function is an alias of.
        Ex: Alias('DrawArraysEXT') = 'DrawArrays'.
        """
        d = GetFunctionDict()
        if d[funcName].aliasprefix == '':
            return "_mesa_"
        else:
            return d[funcName].aliasprefix

def ReverseAlias(funcName):
        """Like Alias(), but the inverse."""
        d = GetFunctionDict()
        if funcName in __ReverseAliases.keys():
                return __ReverseAliases[funcName]
        else:
                return ''

def NonVectorFunction(funcName):
        """Return the non-vector version of the given function, or ''.
        For example: NonVectorFunction("Color3fv") = "Color3f"."""
        d = GetFunctionDict()
        return d[funcName].vectoralias

def ConversionFunction(funcName):
        """Return a function that can be used to implement the
        given function, using different types.
        For example: ConvertedFunction("Color4x") = "Color4f"."""
        d = GetFunctionDict()
        return d[funcName].convertalias

def VectorFunction(funcName):
        """Return the vector version of the given non-vector-valued function,
        or ''.
        For example: VectorVersion("Color3f") = "Color3fv"."""
        d = GetFunctionDict()
        if funcName in __VectorVersion.keys():
                return __VectorVersion[funcName]
        else:
                return ''

def GetCategoryWrapper(func_name):
        """Return a C preprocessor token to test in order to wrap code.
        This handles extensions.
        Example: GetTestWrapper("glActiveTextureARB") = "CR_multitexture"
        Example: GetTestWrapper("glBegin") = ""
        """
        cat = Category(func_name)
        if (cat == "1.0" or
                cat == "1.1" or
                cat == "1.2" or
                cat == "Chromium" or
                cat == "GL_chromium"):
                return ''
        elif cat[0] =='1':
                # i.e. OpenGL 1.3 or 1.4 or 1.5
                return "OPENGL_VERSION_" + string.replace(cat, ".", "_")
        else:
                assert cat != ''
                return string.replace(cat, "GL_", "")


def CanCompile(funcName):
        """Return 1 if the function can be compiled into display lists, else 0."""
        props = Properties(funcName)
        if ("nolist" in props or
                "get" in props or
                "setclient" in props):
                return 0
        else:
                return 1

def HasChromiumProperty(funcName, propertyList):
        """Return 1 if the function or any alias has any property in the
        propertyList"""
        for funcAlias in [funcName, NonVectorFunction(funcName), VectorFunction(funcName)]:
                if funcAlias:
                        props = ChromiumProps(funcAlias)
                        for p in propertyList:
                                if p in props:
                                        return 1
        return 0

def CanPack(funcName):
        """Return 1 if the function can be packed, else 0."""
        return HasChromiumProperty(funcName, ['pack', 'extpack', 'expandpack'])

def HasPackOpcode(funcName):
        """Return 1 if the function has a true pack opcode"""
        return HasChromiumProperty(funcName, ['pack', 'extpack'])

def SetsState(funcName):
        """Return 1 if the function sets server-side state, else 0."""
        props = Properties(funcName)

        # Exceptions.  The first set of these functions *do* have 
        # server-side state-changing  effects, but will be missed 
        # by the general query, because they either render (e.g. 
        # Bitmap) or do not compile into display lists (e.g. all the others).
        # 
        # The second set do *not* have server-side state-changing
        # effects, despite the fact that they do not render
        # and can be compiled.  They are control functions
        # that are not trackable via state.
        if funcName in ['Bitmap', 'DeleteTextures', 'FeedbackBuffer', 
                'RenderMode', 'BindBufferARB', 'DeleteFencesNV']:
                return 1
        elif funcName in ['ExecuteProgramNV']:
                return 0

        # All compilable functions that do not render and that do
        # not set or use client-side state (e.g. DrawArrays, et al.), set
        # server-side state.
        if CanCompile(funcName) and "render" not in props and "useclient" not in props and "setclient" not in props:
                return 1

        # All others don't set server-side state.
        return 0

def SetsClientState(funcName):
        """Return 1 if the function sets client-side state, else 0."""
        props = Properties(funcName)
        if "setclient" in props:
                return 1
        return 0

def SetsTrackedState(funcName):
        """Return 1 if the function sets state that is tracked by
        the state tracker, else 0."""
        # These functions set state, but aren't tracked by the state
        # tracker for various reasons: 
        # - because the state tracker doesn't manage display lists
        #   (e.g. CallList and CallLists)
        # - because the client doesn't have information about what
        #   the server supports, so the function has to go to the
        #   server (e.g. CompressedTexImage calls)
        # - because they require a round-trip to the server (e.g.
        #   the CopyTexImage calls, SetFenceNV, TrackMatrixNV)
        if funcName in [
                'CopyTexImage1D', 'CopyTexImage2D',
                'CopyTexSubImage1D', 'CopyTexSubImage2D', 'CopyTexSubImage3D',
                'CallList', 'CallLists',
                'CompressedTexImage1DARB', 'CompressedTexSubImage1DARB',
                'CompressedTexImage2DARB', 'CompressedTexSubImage2DARB',
                'CompressedTexImage3DARB', 'CompressedTexSubImage3DARB',
                'SetFenceNV'
                ]:
                return 0

        # Anything else that affects client-side state is trackable.
        if SetsClientState(funcName):
                return 1

        # Anything else that doesn't set state at all is certainly
        # not trackable.
        if not SetsState(funcName):
                return 0

        # Per-vertex state isn't tracked the way other state is
        # tracked, so it is specifically excluded.
        if "pervertex" in Properties(funcName):
                return 0

        # Everything else is fine
        return 1

def UsesClientState(funcName):
        """Return 1 if the function uses client-side state, else 0."""
        props = Properties(funcName)
        if "pixelstore" in props or "useclient" in props:
                return 1
        return 0

def IsQuery(funcName):
        """Return 1 if the function returns information to the user, else 0."""
        props = Properties(funcName)
        if "get" in props:
                return 1
        return 0

def FuncGetsState(funcName):
        """Return 1 if the function gets GL state, else 0."""
        d = GetFunctionDict()
        props = Properties(funcName)
        if "get" in props:
                return 1
        else:
                return 0

def IsPointer(dataType):
        """Determine if the datatype is a pointer.  Return 1 or 0."""
        if string.find(dataType, "*") == -1:
                return 0
        else:
                return 1


def PointerType(pointerType):
        """Return the type of a pointer.
        Ex: PointerType('const GLubyte *') = 'GLubyte'
        """
        t = string.split(pointerType, ' ')
        if t[0] == "const":
                t[0] = t[1]
        return t[0]




def OpcodeName(funcName):
        """Return the C token for the opcode for the given function."""
        return "CR_" + string.upper(funcName) + "_OPCODE"


def ExtendedOpcodeName(funcName):
        """Return the C token for the extended opcode for the given function."""
        return "CR_" + string.upper(funcName) + "_EXTEND_OPCODE"




#======================================================================

def MakeCallString(params):
        """Given a list of (name, type, vectorSize) parameters, make a C-style
        formal parameter string.
        Ex return: 'index, x, y, z'.
        """
        result = ''
        i = 1
        n = len(params)
        for (name, type, vecSize, convertToType, validValues, valueConversion) in params:
                result += name
                if i < n:
                        result = result + ', '
                i += 1
        #endfor
        return result
#enddef


def MakeDeclarationString(params):
        """Given a list of (name, type, vectorSize) parameters, make a C-style
        parameter declaration string.
        Ex return: 'GLuint index, GLfloat x, GLfloat y, GLfloat z'.
        """
        n = len(params)
        if n == 0:
                return 'void'
        else:
                result = ''
                i = 1
                for (name, type, vecSize, convertToType, validValues, valueConversion) in params:
                        result = result + type + ' ' + name
                        if i < n:
                                result = result + ', '
                        i += 1
                #endfor
                return result
        #endif
#enddef


def MakePrototypeString(params):
        """Given a list of (name, type, vectorSize) parameters, make a C-style
        parameter prototype string (types only).
        Ex return: 'GLuint, GLfloat, GLfloat, GLfloat'.
        """
        n = len(params)
        if n == 0:
                return 'void'
        else:
                result = ''
                i = 1
                for (name, type, vecSize, convertToType, validValues, valueConversion) in params:
                        result = result + type
                        # see if we need a comma separator
                        if i < n:
                                result = result + ', '
                        i += 1
                #endfor
                return result
        #endif
#enddef


#======================================================================
        
__lengths = {
        'GLbyte': 1,
        'GLubyte': 1,
        'GLshort': 2,
        'GLushort': 2,
        'GLint': 4,
        'GLuint': 4,
        'GLfloat': 4,
        'GLclampf': 4,
        'GLdouble': 8,
        'GLclampd': 8,
        'GLenum': 4,
        'GLboolean': 1,
        'GLsizei': 4,
        'GLbitfield': 4,
        'void': 0,  # XXX why?
        'int': 4,
        'GLintptrARB': 4,   # XXX or 8 bytes?
        'GLsizeiptrARB': 4  # XXX or 8 bytes?
}

def sizeof(type):
        """Return size of C datatype, in bytes."""
        if not type in __lengths.keys():
                print >>sys.stderr, "%s not in lengths!" % type
        return __lengths[type]


#======================================================================
align_types = 1

def FixAlignment( pos, alignment ):
        # if we want double-alignment take word-alignment instead,
        # yes, this is super-lame, but we know what we are doing
        if alignment > 4:
                alignment = 4
        if align_types and alignment and ( pos % alignment ):
                pos += alignment - ( pos % alignment )
        return pos

def WordAlign( pos ):
        return FixAlignment( pos, 4 )

def PointerSize():
        return 8 # Leave room for a 64 bit pointer

def PacketLength( params ):
        len = 0
        for (name, type, vecSize, convertToType, validValues, valueConversion) in params:
                if IsPointer(type):
                        size = PointerSize()
                else:
                        assert string.find(type, "const") == -1
                        size = sizeof(type)
                len = FixAlignment( len, size ) + size
        len = WordAlign( len )
        return len

#======================================================================

__specials = {}

def LoadSpecials( filename ):
        table = {}
        try:
                f = open( filename, "r" )
        except:
                __specials[filename] = {}
                print >>sys.stderr, "%s not present" % filename
                return {}
        
        for line in f.readlines():
                line = string.strip(line)
                if line == "" or line[0] == '#':
                        continue
                table[line] = 1
        
        __specials[filename] = table
        return table


def FindSpecial( filename, glName ):
        table = {}
        try:
                table = __specials[filename]
        except KeyError:
                table = LoadSpecials( filename )
        
        try:
                if (table[glName] == 1):
                        return 1
                else:
                        return 0 #should never happen
        except KeyError:
                return 0


def AllSpecials( table_file ):
        table = {}
        filename = table_file + "_special"
        try:
                table = __specials[filename]
        except KeyError:
                table = LoadSpecials( filename )
        
        keys = table.keys()
        keys.sort()
        return keys


def AllSpecials( table_file ):
        filename = table_file + "_special"
        table = {}
        try:
                table = __specials[filename]
        except KeyError:
                table = LoadSpecials(filename)
        
        ret = table.keys()
        ret.sort()
        return ret
        

def NumSpecials( table_file ):
        filename = table_file + "_special"
        table = {}
        try:
                table = __specials[filename]
        except KeyError:
                table = LoadSpecials(filename)
        return len(table.keys())

def PrintRecord(record):
        argList = MakeDeclarationString(record.params)
        if record.category == "Chromium":
                prefix = "cr"
        else:
                prefix = "gl"
        print '%s %s%s(%s);' % (record.returnType, prefix, record.name, argList )
        if len(record.props) > 0:
                print '   /* %s */' % string.join(record.props, ' ')

#ProcessSpecFile("APIspec.txt", PrintRecord)

