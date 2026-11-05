TurboCodes library {#turbocodes_page}
==================

[TOC]

TurboCodes implements the error-correcting layer of the Strengthened Fuzzy Extractor (SFE): systematic Turbo Codes with a
soft-input iterative decoder \cite berrou1993near, the digital modulations (constellations) that embed codeword bits into
biometric features, and the readers of the text files used by the SFE software.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2025

Why soft-decoded Turbo Codes {#turbocodes_why}
============================

In a fuzzy extractor \cite dodis2008fuzzy the secret is the message of an error-correcting code, and the differences between the
enrollment and probe biometric samples act as channel errors. Classical constructions binarize the features and use a
hard-decision code (BCH, Reed–Solomon). Binarization throws away the reliability of every feature, and the code must be
dimensioned for the worst case. The SFE instead keeps the continuous received values and models each feature as an
individual noisy channel \cite argonesrua2019efficient, \cite argonesrua2024strengthened. In the SFE:

- every codeword bit is carried by one *symbol* (a feature, or a combination of features) with its own modulation and noise
  level;
- the decoder computes the channel likelihoods of every bit from the received symbols and the per-bit conditional pdfs, so
  reliable features weigh more than unreliable ones;
- Turbo Codes approach the channel capacity with iterative decoding \cite berrou1993near, \cite hagenauer1996iterative, and their
  length and rate can be adjusted per user by puncturing \cite hagenauer1988rcpc.

Turbo Codes and modulation constellations were first combined for biometric template protection in
\cite maiorana2012turbo.

Turbo Code {#turbocodes_code}
==========

\ref TurboCode is a parallel concatenation of \f$N_g\f$ recursive systematic convolutional (RSC) generators. Generator
\f$g\f$ is defined by a feedback polynomial \f$a_g(x)\f$ and a feed-forward polynomial \f$b_g(x)\f$, and receives a copy of the
\f$N_I\f$-bit message \f$\mathbf{b}\f$ permuted by its interleaver \f$\mathbf{I}^g\f$. The codeword is
\f[
  \mathbf{o} = (\mathbf{b}, \mathbf{r}_0, \ldots, \mathbf{r}_{N_g-1}),
\f]
where \f$\mathbf{r}_g\f$ is the redundancy of generator \f$g\f$ after puncturing with the pattern \f$\mathbf{p}^g\f$. Without
puncturing the rate is \f$1/(N_g+1)\f$. The complete notation and the derivation of the decoder are given in \ref turbocodes_readme.

Configuration {#turbocodes_config}
-------------

A code is read from a text file by \ref ::parseTurboCodeConfigFile. The file contains the number of generators, the two polynomials of each
generator, one puncturing pattern per generator and one interleaver per generator. The folder \c configFiles holds the best
codes found for rates 1/3, 1/4, 1/5 and 1/6 (\c best-found-turbo-code_rate_1_R.txt); they are installed to
\c PREFIX/share/TurboCodes/configFiles. The fuzzy extractors take only the generators from these files: interleavers and
puncturing patterns are generated per user (see the HSTCFE library).

Iterative MAP decoding {#turbocodes_decoding}
----------------------

\ref TurboCode::decode implements the iterative decoder:

1. \ref TurboCode::computeSymbolPdfEvaluations evaluates, for every codeword bit \f$o_i\f$, the log-likelihoods
   \f$\log f(R_{c(i)} \mid o_i = 0)\f$ and \f$\log f(R_{c(i)} \mid o_i = 1)\f$ of the symbol \f$R_{c(i)}\f$ that carries it
   (\ref TurboCode::setSymbolIndexes, \ref TurboCode::setPdfs0, \ref TurboCode::setPdfs1). The pdfs are gmmlite::GMM mixtures, so
   any modulation and noise model can be used.
2. For each generator, \ref TurboCode::computeGammaMatrix, \ref TurboCode::computeAlphaMatrix and
   \ref TurboCode::computeBetaMatrix run the forward–backward (BCJR) recursions on the trellis \cite bahl1974optimal, using the
   a-priori information from the other generators, and \ref TurboCode::computeLogLikelihoodVector produces the a-posteriori
   log-likelihood ratios of the message bits.
3. The extrinsic part of these ratios becomes the a-priori information of the next generator \cite hagenauer1996iterative.
4. The iterations stop when the hard decisions of all generators agree (\ref TurboCode::decodingConverged) or after the
   maximum number of iterations. The decoded bit is the sign of the final log-likelihood ratio.

Modulations {#turbocodes_modulations}
===========

A \ref Constellation maps \f$m\f$ codeword bits onto a symbol and builds, from a noise model, the pdfs of the symbol
conditioned on each of those bits (\ref Constellation::pdfs0, \ref Constellation::pdfs1).

PSK {#turbocodes_psk}
---

\ref PSK maps \f$m\f$ bits onto the phase \f$\varphi = 2\pi k / 2^m\f$, where \f$k\f$ is the index whose Gray code
\cite gray1953pulse equals the bits, so that neighbouring phases differ in one bit \cite proakis2008digital. It is used for the
*code-offset* features of the SFE, whose normalized values are phases in \f$[0, 2\pi]\f$. With Gaussian phase noise of
standard deviation \f$\sigma\f$, the pdf of the received phase conditioned on bit \f$j\f$ being \f$v\f$ is the mixture of the
noise model centred at every constellation phase whose Gray label has value \f$v\f$ at position \f$j\f$. Every component is
replicated at \f$\varphi - 2\pi\f$, \f$\varphi\f$ and \f$\varphi + 2\pi\f$, a three-term approximation of the wrapped normal
distribution \cite mardia2000directional (\ref PSK::setupConditionalPdfs).

2-PAM {#turbocodes_pam2}
-----

\ref PAM2 maps one bit to \f$-1\f$ (bit 0) or \f$+1\f$ (bit 1). It is used for the *fuzzy-commitment* features of the SFE
\cite juels1999fuzzy, whose committed linear combinations have expected value \f$\pm 1\f$. The conditional pdfs are the noise
model centred at \f$-1\f$ and \f$+1\f$.

File readers and helpers {#turbocodes_files}
========================

vectorfiles.h provides the parsers of the text formats used by the SFE software: samples files (\ref parseSamplesFile, and
\ref ::readSamplesFilesFromPath for a whole folder), training-set files, Turbo Code configuration files
(\ref ::parseTurboCodeConfigFile), sample statistics with Armadillo \cite sanderson2016armadillo (\ref computeSampleStatistics)
and printing helpers. turbo-codes_types.h defines the container aliases shared by all the SFE libraries and the alias
<tt>typedef gmmlite::GMM GMM;</tt>.

Turbo Code notation and MAP decoding {#turbocodes_readme}
====================================

This page defines the notation used by the \ref TurboCode class and derives the soft-input iterative decoder it implements. Turbo Codes are parallel-concatenated recursive systematic convolutional codes \cite berrou1993near.

The iterative decoding algorithm is the MAP method introduced in \cite berrou1993near, in which each constituent decoder computes the a-posteriori probabilities of the input bits with the forward-backward (BCJR) recursions \cite bahl1974optimal and passes extrinsic information to the other decoders \cite hagenauer1996iterative.

Definitions and notation {#turbocodes_definitions}
------------------------

Here we introduce some notation and definitions from Turbo-Codes theory that will become handy when defining the attributes and methods of the class TurboCode.

### Generator
\f[
G = \left(1 \mid \mathbf{g}(x) \right)
\f]
with:
\f[
\mathbf{g}(x) = \left(
\begin{array}{c}
g_0(x) \\
\vdots \\
g_{N_g - 1}(x)
\end{array}\right)
\f]
where \f$g_k(x) = \frac{b_k(x)}{a_k(x)}\f$, with \f$b_k(x)\f$ the feed-forward, and \f$a_k(x)\f$ the feed-back polynomials, with a maximum degree \f$D_k\f$.

### Input sequence

\f[
\mathbf{b} = \left(b_0,\ldots,b_{N_I - 1}\right)
\f]
Note: this input sequence is permutted (by the interleaver) into the convolutional codes input sequences \f$X^g_t =\left[X^g_0,\ldots,X^g_{N_I-1}\right]\f$, with \f$X^g_t = b_{I^g_t}\f$, where \f$I^g\f$ is the \f$N_I\textrm{-permutation}\f$ applied to the input of the generator \f$g\f$.

### State sequence

\f[
\mathbf{S}^g = \left(S^g_{-1},\ldots,S^g_{N_I}\right)
\f]

Note that \f$S^g_t \in \{0,2^{D_g}-1\}\f$, and \f$S^g_t = f^g(X^g_t,S^g_{t-1})\f$, where \f$f^g\f$ is determined by the feed-back part of the \f$g\f$-th generator. Also, \f$S_{-1}^g = 0\f$.

### Puncturing patterns

\f[
\mathbf{p}(x) = \left(
\begin{array}{c}
p^0(x) \\
\vdots \\
p^{N_g-1}(x)
\end{array}\right)
\f]
with \f$ p^g(x) \f$ a polynomial of degree \f$N_I - 1\f$, noted as \f$p^g_0 + p^g_1 x + \cdots + p^g_{N_I - 1} x^{N_I - 1}\f$. The redundancy generated by \f$X^g_t\f$ is appended to the output only if \f$ p^g_t = 1\f$.

### Interleavers
\f[
\mathbf{I} = \left(
\begin{array}{c}
\mathbf{I}^0 \\
\vdots \\
\mathbf{I}^{N_g-1}
\end{array}\right)
\f]
with \f$\mathbf{I}^g\f$ a permutation of \f$N_I\f$. The input to generator \f$g\f$ is constructed by setting \f$X^g_t = b_{I^g_t}\f$. The deinterleavers can be easily defined from the interleavers.

### Output sequence

The binary output sequence of the TurboCode is the concatenation of the input sequence \f$\mathbf{b}\f$ and the permuted and punctured redundancies from the\f$N_g\f$ convolutional codes.
\f[
\mathbf{o} = \left( \mathbf{b}, \mathbf{r}_0, \ldots, \mathbf{r}_{N_g-1} \right) = \left(o_0, \ldots, o_{N_o-1}\right)\,\textrm{,}
\f]
where \f$N_o\f$ is the codeword length, taking puncturing into account. If we define the redundancies of the \f$i\textrm{-th}\f$ convolutional code as:
\f[
\mathbf{Y}^g = \left(Y_0^g, \ldots, Y^g_{N_I-1}\right)
\f]
then \f$\mathbf{r}_g\f$ is the puncturing of \f$\mathbf{Y}^g\f$.

### Channel Output sequence

It is the result of encoding \f$\mathbf{o}\f$, ready for channel transmission. It is defined as:

\f[
\mathbf{Z} = \left(Z_0, \ldots, Z_{C-1}\right)
\f]

For each \f$Z_c\f$, there are a set \f$\mathcal{S}_c\f$ of bits in the output sequence \f$\mathbf{o}\f$ that fully determines \f$Z_c\f$, i.e.:
\f[
f_{Z_c}(z) = \prod_{o_i}^{\mathcal{S}_c} { \sum_{x}^{\{0,1\}}{ f_{Z_c \mid o_i = x}\left(z\right) \mathrm{P}\left(o_i = x\right) } }
\f]

### Channel Input sequence

It is the received channel signal. It is defined as:

\f[
\mathbf{R} = \left(R_0, \ldots, R_{C-1}\right)
\f]

The probability density of \f$R_i\f$ is fully determined by the conditional probability densities \f$f_{R_i \mid o_k}(R)\f$, with \f$k \in \mathcal{S}_i\f$.

We define the indicator functions for each convolutional encoder \f$g\f$:

\f[
\textrm{indB}^g(i) = c \mid Z_c \textrm{ encodes } X^g_i
\f]

and:

\f[
\textrm{indR}^g(i) = c \mid Z_c \textrm{ encodes } Y^g_i
\f]

Then we can define the decoding input sequence for each convolutional decoder as:

\f[
\mathbf{d}^g = \left(d_0, \ldots, d_{N_I-1}\right)
\f]
with \f$d^g_t = \left\{R_{\textrm{indB}^g(t)},R_{\textrm{indR}^g(t)}\right\}\f$.

### Notation

For each convolutional encoder \f$g\f$ we note:

\f[
\lambda^g_t(s) = \mathrm{P}(S^g_t=s,\mathbf{d}^g)
\f]

\f[
\sigma^g_t(s',s) = \mathrm{P}(S^g_{t-1}=s',S^g_t=s,\mathbf{d}^g)
\f]

\f[
\alpha^g_t(s) = \mathrm{P}(S^g_t=s,\mathbf{d}^g_{0,t})
\f]

\f[
\beta^g_t(s) = \mathrm{P}(\mathbf{d}^g_{t+1,N_I-1} \mid S^g_t=s)
\f]

\f[
\gamma^g_t(s',s) = \mathrm{P}(S^g_t=s, \mathbf{d}^g_t \mid S^g_{t-1}=s')
\f]

\f[
q^g_t(X \mid s',s) = \mathrm{P}(X^g_t = X \mid S^g_{t-1}=s',S^g_t=s)
\f]

Their likelihood are noted as their capital versions \f$\Lambda, \Sigma, \ldots\f$.

### Derivations

We have:

\f[
q^g_t(0 \mid s',s) = \left\{
\begin{array}{cl}
1 & \textrm{if } s=\lfloor\frac{s'}{2}\rfloor \\
0 & \textrm{otherwise.}
\end{array}
\right.
\f]
and
\f[
q^g_t(1 \mid s',s) = \left\{
\begin{array}{cl}
1 & \textrm{if } s=2^{{D_g}-1} + \lfloor\frac{s'}{2}\rfloor \\
0 & \textrm{otherwise.}
\end{array}
\right.
\f]

And it can be derived:
\f[
\lambda^g_t(s) = \alpha^g_t(s) \beta^g_t(s)
\f]

\f[
\sigma^g_t(s',s) = \alpha^g_{t-1}(s') \gamma^g_t(s',s) \beta^g_t(s)
\f]

\f[
\alpha^g_t(s) = \sum_{s'}{
\alpha^g_{t-1}(s')\gamma^g_t(s',s)
}
\f]

\f[
\beta^g_t(s) = \sum_{s'}{
\beta^g_{t+1}(s')\gamma^g_{t+1}(s,s')
}
\f]

And

\f[
\gamma^g_t(s',s) = \mathrm{P}(S^g_t = s \mid S^g_{t-1} = s') \sum_{X}^{\{0,1\}}{q^g_t(X \mid s',s) f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = X}(R_{\mathrm{indB}^g(t)}) f_{R_{\mathrm{indR}^g(t)} \mid X^g_t = X}(R_{\mathrm{indR}^g(t)}) }
\f]

Where \f$\mathrm{P}(S^g_t = s \mid S^g_{t-1} = s')\f$ is the a-priori information on \f$X^g_t\f$ and \f$X^g_{t-1}\f$. These terms are initially \f$\frac{1}{2}\f$, but they are updated after each decoding iteration from the extrinsic loglikelihood ratio from other decoders.

We can define
\f{eqnarray*}{
\gamma^g_{t,x}(s',s) & = & \mathrm{P}(S^g_t=s,\mathbf{d}^g_t \mid S^g_{t-1}=s', X^g_t=x) \\
& = & \mathrm{P}(S^g_t = s \mid S^g_{t-1} = s') {q^g_t(x \mid s',s) f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = x}(R_{\mathrm{indB}^g(t)}) f_{R_{\mathrm{indR}^g(t)} \mid X^g_t = x}(R_{\mathrm{indR}^g(t)}) }
\f}
where \f$\gamma^g_{t}(s',s) = \gamma^g_{t,0}(s',s) + \gamma^g_{t,1}(s',s)\f$

We must note that for the possible state sequences \f$(S_{t-1}, S_t)\f$, there is only one possible \f$S_t\f$. This makes either \f$q^g_t\left(0 \mid s',s\right) = 0\f$ or \f$q^g_t\left(1 \mid s',s\right) = 0\f$. This makes:
\f[
\gamma^g_{t}(s',s) = \left\{
\begin{array}{ll}
\gamma^g_{t,0}(s',s) & \textrm{if } q^g_t\left(0 \mid s',s\right) = 1 \\
\gamma^g_{t,1}(s',s) & \textrm{if } q^g_t\left(1 \mid s',s\right) = 1 \\
0 & \textrm{otherwise.}
\end{array}
\right.
\f]

Since the Turbo-Code is systematic, \f$ f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = x}(R_{\mathrm{indB}^g(t)}) \f$ is independent of \f$S_t\f$ and \f$S_{t-1}\f$.

If we define
\f{eqnarray*}{
{\gamma'}^g_{t,x}(s',s) & = & \mathrm{P}(S^g_t=s,R_{\mathrm{indR}^g(t)} \mid S^g_{t-1}=s', X^g_t=x) \\
& = & \mathrm{P}(S^g_t = s \mid S^g_{t-1} = s') {q^g_t(x \mid s',s) f_{R_{\mathrm{indR}^g(t)} \mid X^g_t = x}(R_{\mathrm{indR}^g(t)}) } \,\textrm{,}
\f}
then the likelihood ratio for each convolutional code \f$g\f$ is:

\f[
\textbf{LR}^g = \left(\textrm{LR}^g_1,\ldots,\textrm{LR}^g_{N_I}\right)
\f]
with:
\f{eqnarray*}{
\textrm{LR}^g_t & = & \frac{\sum_s \sum_{s'} \alpha^g_{t-1}(s') \gamma^g_{t,1}(s',s) \beta^g_t(s)}{\sum_s \sum_{s'} \alpha^g_{t-1}(s') \gamma^g_{t,0}(s',s) \beta^g_t(s)} \\
& = & \frac{ f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = 1}(R_{\mathrm{indB}^g(t)}) \sum_s \sum_{s'} \alpha^g_{t-1}(s') {\gamma'}^g_{t,1}(s',s) \beta^g_t(s)}{f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = 0}(R_{\mathrm{indB}^g(t)}) \sum_s \sum_{s'} \alpha^g_{t-1}(s') {\gamma'}^g_{t,0}(s',s) \beta^g_t(s)} \,\textrm{,}
\f}
and
\f{eqnarray*}{
\textrm{LLR}^g_t & = & \mathrm{ln}\left( \frac{\sum_s \sum_{s'} \alpha^g_{t-1}(s') \gamma^g_{t,1}(s',s) \beta^g_t(s)}{\sum_s \sum_{s'} \alpha^g_{t-1}(s') \gamma^g_{t,0}(s',s) \beta^g_t(s)}\right) \\
& = & \mathrm{ln}\frac{f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = 1}(R_{\mathrm{indB}^g(t)})}{f_{R_{\mathrm{indB}^b(t)} \mid X^g_t = 0}(R_{\mathrm{indB}^g(t)})} + \mathrm{ln}\frac{  \sum_s \sum_{s'} \alpha^g_{t-1}(s') {\gamma'}^g_{t,1}(s',s) \beta^g_t(s)}{ \sum_s \sum_{s'} \alpha^g_{t-1}(s') {\gamma'}^g_{t,0}(s',s) \beta^g_t(s)} \,\textrm{,}
\f}
where the first term is the intrinsic \f$\textrm{LLR}\f$ for generator \f$g\f$ at index \f$t\f$, noted as \f${\textrm{LLR}_{\textrm{in}}}^g_t\f$, and the second term is the extrinsic \f$\textrm{LLR}\f$ for generator \f$g\f$ at index \f$t\f$, noted as \f${\textrm{LLR}_{\textrm{ex}}}^g_t\f$. Then, we have:
\f[
\textbf{LLR}^g =  {\textbf{LLR}_{\textrm{in}}}^g + {\textbf{LLR}_{\textrm{ex}}}^g
\f]

Building {#turbocodes_build}
========

TurboCodes needs a C++17 compiler, GMMLite and Armadillo. It is built by the top-level CMake project of the repository, or on its
own against an installed GMMLite:

    cmake -S libraries/TurboCodes -B build/TurboCodes -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=PREFIX -DCMAKE_INSTALL_PREFIX=PREFIX
    cmake --build build/TurboCodes -j
    cmake --install build/TurboCodes

It is installed as \c libTurboCodes (\c libTurboCodes_Debug in Debug builds), with its headers in \c PREFIX/include/TurboCodes
and the configuration files in \c PREFIX/share/TurboCodes/configFiles.
