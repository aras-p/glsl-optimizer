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
				<link rel="stylesheet" type="text/css" href="trace.css"/>
			</head>
			<body>
				<ul class="calls">
					<xsl:apply-templates/>
				</ul>
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
			<ul class="args">
				<xsl:apply-templates select="arg"/>
			</ul>
			<xsl:text>)</xsl:text>
			<xsl:apply-templates select="ret"/>
		</li>
	</xsl:template>

	<xsl:template match="arg|member">
		<li>
			<xsl:apply-templates select="@name"/>
			<xsl:text> = </xsl:text>
			<xsl:apply-templates />
			<xsl:if test="position() != last()">
				<xsl:text>, </xsl:text>
			</xsl:if>
		</li>
	</xsl:template>

	<xsl:template match="ret">
		<xsl:text> = </xsl:text>
		<xsl:apply-templates />
	</xsl:template>

	<xsl:template match="bool|int|uint">
		<span class="lit">
			<xsl:value-of select="text()"/>
		</span>
	</xsl:template>

	<xsl:template match="string">
		<span class="lit">
			<xsl:text>"</xsl:text>
			<xsl:value-of select="text()"/>
			<xsl:text>"</xsl:text>
		</span>
	</xsl:template>

	<xsl:template match="array|struct">
		<xsl:text>{</xsl:text>
		<xsl:apply-templates />
		<xsl:text>}</xsl:text>
	</xsl:template>

	<xsl:template match="elem">
		<li>
			<xsl:apply-templates />
			<xsl:if test="position() != last()">
				<xsl:text>, </xsl:text>
			</xsl:if>
		</li>
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
</xsl:transform>
