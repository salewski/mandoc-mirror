<?xml version='1.0' encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
  <xsl:output encoding="iso-8859-15" method="html" indent="yes" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" />
  <xsl:template match="/changelog">
    <html>
    <head>
      <title>mdocml - CVS-ChangeLog</title>
      <style type="text/css">
      	h3 { background-color: #E6E6FA; color: #000000; padding: 2px; }
	.rev { color: #808080 }
      </style>
    </head>
      <body>
          <xsl:for-each select="entry">
              <h3>
	      	<xsl:text>Files modified by </xsl:text>
                <xsl:value-of select="concat(author, ': ', date, ' (', time, ')')" />
              </h3>
	      <strong>
	      	<xsl:text>Note: </xsl:text>
	      </strong>
	      	<xsl:value-of select="msg"/>
              <ul>
                <xsl:for-each select="file">
		 <li>
                  <xsl:value-of select="name"/>
		  <span class="rev">
		  <xsl:text> - Rev: </xsl:text>
		  <xsl:value-of select="revision"/>
		  <xsl:text>, Status: </xsl:text>
		  <xsl:value-of select="cvsstate"/>
		  </span>
		 </li>
                </xsl:for-each>
              </ul>
          </xsl:for-each>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
