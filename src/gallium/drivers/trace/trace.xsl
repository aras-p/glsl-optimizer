<?xml version="1.0"?>

<!--

Copyright 2008 Tungsten Graphics, Inc.

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

!-->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<xsl:output method="html" />

	<xsl:strip-space elements="*" />

	<xsl:template match="/trace">
		<html>
			<head>
				<title>Gallium Trace</title>
			</head>
			<style>
				body {
					font-family: verdana, sans-serif;
					font-size: 11px;
					font-weight: normal;
					text-align : left;
				}

				.fun {
					font-weight: bold;
				}

				.var {
					font-style: italic;
				}

				.typ {
					display: none;
				}

				.lit {
					color: #0000ff;
				}

				.ptr {
					color: #008000;
				}
			</style>
			<body>
				<ol class="calls">
					<xsl:apply-templates/>
				</ol>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="call">
		<li>
			<span class="fun">
				<xsl:value-of select="@class"/>
				<xsl:text>::</xsl:text>
				<xsl:value-of select="@method"/>
			</span>
			<xsl:text>(</xsl:text>
			<xsl:apply-templates select="arg"/>
			<xsl:text>)</xsl:text>
			<xsl:apply-templates select="ret"/>
		</li>
	</xsl:template>

	<xsl:template match="arg|member">
			<xsl:apply-templates select="@name"/>
			<xsl:text> = </xsl:text>
			<xsl:apply-templates />
			<xsl:if test="position() != last()">
				<xsl:text>, </xsl:text>
			</xsl:if>
	</xsl:template>

	<xsl:template match="ret">
		<xsl:text> = </xsl:text>
		<xsl:apply-templates />
	</xsl:template>

	<xsl:template match="bool|int|uint|float|enum">
		<span class="lit">
			<xsl:value-of select="text()"/>
		</span>
	</xsl:template>

	<xsl:template match="bytes">
		<span class="lit">
			<xsl:text>...</xsl:text>
		</span>
	</xsl:template>

	<xsl:template match="string">
		<span class="lit">
			<xsl:text>"</xsl:text>
			<xsl:call-template name="break">
				<xsl:with-param name="text" select="text()"/>
			</xsl:call-template>
			<xsl:text>"</xsl:text>
		</span>
	</xsl:template>

	<xsl:template match="array|struct">
		<xsl:text>{</xsl:text>
		<xsl:apply-templates />
		<xsl:text>}</xsl:text>
	</xsl:template>

	<xsl:template match="elem">
		<xsl:apply-templates />
		<xsl:if test="position() != last()">
			<xsl:text>, </xsl:text>
		</xsl:if>
	</xsl:template>

	<xsl:template match="null">
		<span class="ptr">
			<xsl:text>NULL</xsl:text>
		</span>
	</xsl:template>

	<xsl:template match="ptr">
		<span class="ptr">
			<xsl:value-of select="text()"/>
		</span>
	</xsl:template>

	<xsl:template match="@name">
		<span class="var">
			<xsl:value-of select="."/>
		</span>
	</xsl:template>
	
	<xsl:template name="break">
		<xsl:param name="text" select="."/>
		<xsl:choose>
			<xsl:when test="contains($text, '&#xa;')">
				<xsl:value-of select="substring-before($text, '&#xa;')"/>
				<br/>
				<xsl:call-template name="break">
					 <xsl:with-param name="text" select="substring-after($text, '&#xa;')"/>
				</xsl:call-template>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="$text"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template name="replace">
		<xsl:param name="text"/>
		<xsl:param name="from"/>
		<xsl:param name="to"/>
		<xsl:choose>
			<xsl:when test="contains($text,$from)">
				<xsl:value-of select="concat(substring-before($text,$from),$to)"/>
				<xsl:call-template name="replace">
					<xsl:with-param name="text" select="substring-after($text,$from)"/>
					<xsl:with-param name="from" select="$from"/>
					<xsl:with-param name="to" select="$to"/>
				</xsl:call-template>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="$text"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

</xsl:transform>
