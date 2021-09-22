Summary: Samba SMB client and server
Name: samba-appliance
Version: 0.2
Release: 1
Copyright: GNU GPL version 2
Group: Networking
Source: %{name}-%{version}-src.tar.gz
Packager: John H Terpstra [Samba-Team] <jht@samba.org>
Requires: pam >= 0.64
Prereq: chkconfig fileutils
BuildRoot: /var/tmp/samba
Provides: winbind

%define prefix /usr/local/samba

%define tng_build_dir $RPM_BUILD_DIR/%{name}-%{version}/tng
%define head_build_dir $RPM_BUILD_DIR/%{name}-%{version}/head

%description
Samba provides an SMB server which can be used to provide
network services to SMB (sometimes called "Lan Manager")
clients, including various versions of MS Windows, OS/2,
and other Linux machines. Samba also provides some SMB
clients, which complement the built-in SMB filesystem
in Linux. Samba uses NetBIOS over TCP/IP (NetBT) protocols
and does NOT need NetBEUI (Microsoft Raw NetBIOS frame)
protocol.

Samba-2 features an almost working NT Domain Control
capability and includes the new SWAT (Samba Web Administration
Tool) that allows samba's smb.conf file to be remotely managed
using your favourite web browser. For the time being this is
being enabled on TCP port 901 via inetd.

Please refer to the WHATSNEW.txt document for fixup information.
This binary release includes encrypted password support.
Please read the smb.conf file and ENCRYPTION.txt in the
docs directory for implementation details.

NOTE: Red Hat Linux 5.X Uses PAM which has integrated support
for Shadow passwords. Do NOT recompile with the SHADOW_PWD option
enabled. Red Hat Linux has built in support for quotas in PAM.

%changelog
* Mon Jun 5 2000 Tim Potter <tpot@samba.org>
 - Modified to use prefix=/usr/local/samba everywhere

* Sat Nov 29 1999 Matthew Vanecek <mev0003@unt.edu>
 - Added a Prefix and changed "/usr" to "%{prefix}"

* Sat Nov 11 1999 Tridge <tridge@linuxcare.com>
 - changed from mount.smb to mount.smbfs

* Sat Oct 9 1999 Tridge <tridge@linuxcare.com>
 - removed smbwrapper
 - added smbmnt and smbmount

* Sun Apr 25 1999 John H Terpstra <jht@samba.org>
 - added smbsh.1 man page

* Fri Mar 26 1999 Andrew Tridgell <tridge@samba.org>
 - added --with-pam as pam is no longer used by default

* Sat Jan 27 1999 Jeremy Allison <jra@samba.org>
 - Removed smbrun binary and tidied up some loose ends

* Sun Oct 25 1998 John H Terpstra <jht@samba.org>
 - Added parameters to /config to ensure smb.conf, lmhosts, 
	and smbusers never gets over-written.

* Sat Oct 24 1998 John H Terpstra <jht@samba.org>
 - removed README.smbsh file from docs area

* Mon Oct 05 1998 John H Terpstra <jht@samba.org>
 - Added rpcclient to binaries list
 - Added smbwrapper stuff

* Fri Aug 21 1998 John H Terpstra <jht@samba.org>
 - Updated for Samba version 2.0 building

* Tue Jul 07 1998 Erik Troan <ewt@redhat.com>
  - updated postun triggerscript to check $0
  - clear /etc/codepages from %preun instead of %postun

* Sat Jul 04 1998 John H Terpstra <jht@samba.org>
 - fixed codepage preservation during update via -Uvh

* Mon Jun 08 1998 Erik Troan <ewt@redhat.com>
  - made the %postun script a tad less agressive; no reason to remove
    the logs or lock file 
  - the %postun and %preun should only exectute if this is the final
    removal
  - migrated %triggerpostun from Red Hat's samba package to work around
    packaging problems in some Red Hat samba releases

* Sun Apr 26 1998 John H Terpstra <jht@samba.org>
 - Tidy up for early alpha releases
 - added findsmb from SGI packaging

* Thu Apr 09 1998 John H Terpstra <jht@samba.org>
 - Updated spec file
 - Included new codepage.936

* Sat Mar 20 1998 John H Terpstra <jht@samba.org>
 - Added swat facility

* Sat Jan 24 1998 John H Terpstra <jht@samba.org>
 - Many optimisations (some suggested by Manoj Kasichainula <manojk@io.com>
  - Use of chkconfig in place of individual symlinks to /etc/rc.d/init/smb
  - Compounded make line
  - Updated smb.init restart mechanism
  - Use compound mkdir -p line instead of individual calls to mkdir
  - Fixed smb.conf file path for log files
  - Fixed smb.conf file path for incoming smb print spool directory
  - Added a number of options to smb.conf file
  - Added smbadduser command (missed from all previous RPMs) - Doooh!
  - Added smbuser file and smb.conf file updates for username map

%prep
%setup

%build
make config
make

%install
rm -rf $RPM_BUILD_ROOT

# Install stuff for tng

mkdir -p $RPM_BUILD_ROOT%{prefix}/bin
mkdir -p $RPM_BUILD_ROOT/lib/security
cp %{tng_build_dir}/bin/samedit  $RPM_BUILD_ROOT%{prefix}/bin
cp %{tng_build_dir}/bin/winbindd $RPM_BUILD_ROOT%{prefix}/bin
cp %{tng_build_dir}/nsswitch/libnss_winbind.so $RPM_BUILD_ROOT/lib
cp %{tng_build_dir}/nsswitch/pam_winbind.so $RPM_BUILD_ROOT/lib/security

# Install stuff for head

mkdir -p $RPM_BUILD_ROOT%{prefix}/lib/codepages/src
mkdir -p $RPM_BUILD_ROOT/etc/{logrotate.d,pam.d}
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/{init.d,rc0.d,rc1.d,rc2.d,rc3.d,rc5.d,rc6.d}
mkdir -p $RPM_BUILD_ROOT%{prefix}/bin
mkdir -p $RPM_BUILD_ROOT%{prefix}/share/swat/{images,help,include}
mkdir -p $RPM_BUILD_ROOT%{prefix}/man/{man1,man5,man7,man8}
mkdir -p $RPM_BUILD_ROOT%{prefix}/var/locks
mkdir -p $RPM_BUILD_ROOT%{prefix}/private

# Install standard binary files
for i in nmblookup smbclient smbspool smbpasswd smbstatus testparm testprns \
      make_smbcodepage make_printerdef
do
install -m755 -s %{head_build_dir}/source/bin/$i $RPM_BUILD_ROOT%{prefix}/bin
done
for i in addtosmbpass mksmbpasswd.sh smbtar 
do
install -m755 %{head_build_dir}/source/script/$i $RPM_BUILD_ROOT%{prefix}/bin
done

# Install secure binary files
for i in smbd nmbd swat smbmount smbmnt smbumount
do
install -m755 -s %{head_build_dir}/source/bin/$i $RPM_BUILD_ROOT%{prefix}/bin
done

# we need a symlink for mount to recognise the smb filesystem type
#ln -sf %{prefix}/bin/smbmount $RPM_BUILD_ROOT%{prefix}/bin/mount.smbfs

# Install level 1 man pages
for i in smbclient.1 smbrun.1 smbstatus.1 smbtar.1 testparm.1 testprns.1 make_smbcodepage.1 nmblookup.1
do
install -m644 %{head_build_dir}/docs/manpages/$i $RPM_BUILD_ROOT%{prefix}/man/man1
done

# Install codepage source files
for i in 437 737 850 852 861 866 932 936 949 950
do
install -m644 %{head_build_dir}/source/codepages/codepage_def.$i $RPM_BUILD_ROOT%{prefix}/lib/codepages/src
done

# Install SWAT helper files
for i in %{head_build_dir}/swat/help/*.html %{head_build_dir}/docs/htmldocs/*.html
do
install -m644 $i $RPM_BUILD_ROOT%{prefix}/share/swat/help
done
for i in %{head_build_dir}/swat/images/*.gif
do
install -m644 $i $RPM_BUILD_ROOT%{prefix}/share/swat/images
done
for i in %{head_build_dir}/swat/include/*.html
do
install -m644 $i $RPM_BUILD_ROOT%{prefix}/share/swat/include
done

# Install the miscellany
install -m644 %{head_build_dir}/swat/README $RPM_BUILD_ROOT%{prefix}/share/swat
install -m644 %{head_build_dir}/docs/manpages/smb.conf.5 $RPM_BUILD_ROOT%{prefix}/man/man5
install -m644 %{head_build_dir}/docs/manpages/lmhosts.5 $RPM_BUILD_ROOT%{prefix}/man/man5
install -m644 %{head_build_dir}/docs/manpages/smbpasswd.5 $RPM_BUILD_ROOT%{prefix}/man/man5
install -m644 %{head_build_dir}/docs/manpages/samba.7 $RPM_BUILD_ROOT%{prefix}/man/man7
install -m644 %{head_build_dir}/docs/manpages/smbd.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 %{head_build_dir}/docs/manpages/nmbd.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 %{head_build_dir}/docs/manpages/swat.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 %{head_build_dir}/docs/manpages/smbmnt.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 %{head_build_dir}/docs/manpages/smbmount.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 %{head_build_dir}/docs/manpages/smbpasswd.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 %{head_build_dir}/docs/manpages/smbspool.8 $RPM_BUILD_ROOT%{prefix}/man/man8
install -m644 $RPM_BUILD_DIR/%{name}-%{version}/smb.conf-appliance $RPM_BUILD_ROOT%{prefix}/lib/smb.conf
install -m644 %{head_build_dir}/packaging/RedHat/smbusers $RPM_BUILD_ROOT/etc/smbusers
install -m755 %{head_build_dir}/packaging/RedHat/smbprint $RPM_BUILD_ROOT%{prefix}/bin
install -m755 %{head_build_dir}/packaging/RedHat/findsmb $RPM_BUILD_ROOT%{prefix}/bin
install -m755 %{head_build_dir}/packaging/RedHat/smbadduser $RPM_BUILD_ROOT%{prefix}/bin
install -m755 %{head_build_dir}/packaging/RedHat/smb.init $RPM_BUILD_ROOT/etc/rc.d/init.d/smb
install -m755 %{head_build_dir}/packaging/RedHat/smb.init $RPM_BUILD_ROOT%{prefix}/bin/samba
install -m644 %{head_build_dir}/packaging/RedHat/samba.pamd $RPM_BUILD_ROOT/etc/pam.d/samba
install -m644 %{head_build_dir}/packaging/RedHat/samba.log $RPM_BUILD_ROOT/etc/logrotate.d/samba
echo 127.0.0.1 localhost > $RPM_BUILD_ROOT/etc/lmhosts

%clean
rm -rf $RPM_BUILD_ROOT

%post
#/sbin/chkconfig --add smb

# Build codepage load files
for i in 437 737 850 852 861 866 932 936 949 950
do
%{prefix}/bin/make_smbcodepage c $i %{prefix}/lib/codepages/src/codepage_def.$i %{prefix}/lib/codepages/codepage.$i
done

# Add swat entry to /etc/services if not already there
if !( grep ^[:space:]*swat /etc/services > /dev/null ) then
	echo 'swat		901/tcp				# Add swat service used via inetd' >> /etc/services
fi

# Add swat entry to /etc/inetd.conf if needed
if !( grep ^[:space:]*swat /etc/inetd.conf > /dev/null ) then
	echo 'swat	stream	tcp	nowait.400	root	%{prefix}/sbin/swat swat' >> /etc/inetd.conf
killall -1 inetd || :
fi

%preun
if [ $1 = 0 ] ; then
    /sbin/chkconfig --del smb

    for n in %{prefix}/lib/codepages/*; do
	if [ $n != %{prefix}/lib/codepages/src ]; then
	    rm -rf $n
	fi
    done
    # We want to remove the browse.dat and wins.dat files so they can not interfer with a new version of samba!
    if [ -e %{prefix}/var/locks/browse.dat ]; then
	    rm -f %{prefix}/var/locks/browse.dat
    fi
    if [ -e %{prefix}/var/locks/wins.dat ]; then
	    rm -f %{prefix}/var/locks/wins.dat
    fi
fi

%postun
# Only delete remnants of samba if this is the final deletion.
if [ $1 = 0 ] ; then
    if [ -x /etc/pam.d/samba ]; then
      rm -f /etc/pam.d/samba
    fi
    if [ -e /var/log/samba ]; then
      rm -rf /var/log/samba
   fi
    if [ -e /var/lock/samba ]; then
      rm -rf /var/lock/samba
    fi

    # Remove swat entries from /etc/inetd.conf and /etc/services
    cd /etc
    tmpfile=/etc/tmp.$$
    sed -e '/^[:space:]*swat.*$/d' /etc/inetd.conf > $tmpfile
    mv $tmpfile inetd.conf
    sed -e '/^[:space:]*swat.*$/d' /etc/services > $tmpfile
    mv $tmpfile services
fi

%triggerpostun -- samba < samba-2.0.0
if [ $0 != 0 ]; then
    /sbin/chkconfig --add smb
fi


%files
%doc %{head_build_dir}/README %{head_build_dir}/COPYING 
%doc %{head_build_dir}/Manifest %{head_build_dir}/Read-Manifest-Now
%doc %{head_build_dir}/WHATSNEW.txt %{head_build_dir}/Roadmap
%doc %{head_build_dir}/docs
%doc %{head_build_dir}/swat/README
%doc %{head_build_dir}/examples
%attr(-,root,root) %{prefix}/bin/smbd
%attr(-,root,root) %{prefix}/bin/nmbd
%attr(-,root,root) %{prefix}/bin/swat
%attr(-,root,root) %{prefix}/bin/smbmnt
%attr(-,root,root) %{prefix}/bin/smbmount
%attr(-,root,root) %{prefix}/bin/smbumount
%attr(0750,root,root) %{prefix}/bin/samba
%attr(-,root,root) %{prefix}/bin/addtosmbpass
%attr(-,root,root) %{prefix}/bin/mksmbpasswd.sh
%attr(-,root,root) %{prefix}/bin/smbclient
%attr(-,root,root) %{prefix}/bin/smbspool
%attr(-,root,root) %{prefix}/bin/testparm
%attr(-,root,root) %{prefix}/bin/testprns
%attr(-,root,root) %{prefix}/bin/findsmb
%attr(-,root,root) %{prefix}/bin/smbstatus
%attr(-,root,root) %{prefix}/bin/nmblookup
%attr(-,root,root) %{prefix}/bin/make_smbcodepage
%attr(-,root,root) %{prefix}/bin/make_printerdef
%attr(-,root,root) %{prefix}/bin/smbpasswd
%attr(-,root,root) %{prefix}/bin/smbtar
%attr(-,root,root) %{prefix}/bin/smbprint
%attr(-,root,root) %{prefix}/bin/smbadduser
%attr(-,root,root) %{prefix}/share/swat/help/welcome.html
%attr(-,root,root) %{prefix}/share/swat/help/DOMAIN_MEMBER.html
%attr(-,root,root) %{prefix}/share/swat/help/NT_Security.html
%attr(-,root,root) %{prefix}/share/swat/help/lmhosts.5.html
%attr(-,root,root) %{prefix}/share/swat/help/make_smbcodepage.1.html
%attr(-,root,root) %{prefix}/share/swat/help/nmbd.8.html
%attr(-,root,root) %{prefix}/share/swat/help/nmblookup.1.html
%attr(-,root,root) %{prefix}/share/swat/help/samba.7.html
%attr(-,root,root) %{prefix}/share/swat/help/smb.conf.5.html
%attr(-,root,root) %{prefix}/share/swat/help/smbclient.1.html
%attr(-,root,root) %{prefix}/share/swat/help/smbspool.8.html
%attr(-,root,root) %{prefix}/share/swat/help/smbd.8.html
%attr(-,root,root) %{prefix}/share/swat/help/smbpasswd.5.html
%attr(-,root,root) %{prefix}/share/swat/help/smbpasswd.8.html
%attr(-,root,root) %{prefix}/share/swat/help/smbrun.1.html
%attr(-,root,root) %{prefix}/share/swat/help/smbstatus.1.html
%attr(-,root,root) %{prefix}/share/swat/help/smbtar.1.html
%attr(-,root,root) %{prefix}/share/swat/help/swat.8.html
%attr(-,root,root) %{prefix}/share/swat/help/testparm.1.html
%attr(-,root,root) %{prefix}/share/swat/help/testprns.1.html
%attr(-,root,root) %{prefix}/share/swat/images/globals.gif
%attr(-,root,root) %{prefix}/share/swat/images/home.gif
%attr(-,root,root) %{prefix}/share/swat/images/passwd.gif
%attr(-,root,root) %{prefix}/share/swat/images/printers.gif
%attr(-,root,root) %{prefix}/share/swat/images/shares.gif
%attr(-,root,root) %{prefix}/share/swat/images/samba.gif
%attr(-,root,root) %{prefix}/share/swat/images/status.gif
%attr(-,root,root) %{prefix}/share/swat/images/viewconfig.gif
%attr(-,root,root) %{prefix}/share/swat/include/header.html
%attr(-,root,root) %{prefix}/share/swat/include/footer.html
%attr(-,root,root) %config(noreplace) /etc/lmhosts
%attr(-,root,root) %config(noreplace) %{prefix}/lib/smb.conf
%attr(-,root,root) %config(noreplace) /etc/smbusers
%attr(-,root,root) /etc/rc.d/init.d/smb
%attr(-,root,root) /etc/logrotate.d/samba
%attr(-,root,root) /etc/pam.d/samba
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.437
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.737
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.850
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.852
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.861
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.866
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.932
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.936
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.949
%attr(-,root,root) %{prefix}/lib/codepages/src/codepage_def.950
%attr(-,root,root) %{prefix}/man/man1/smbstatus.1
%attr(-,root,root) %{prefix}/man/man1/smbclient.1
%attr(-,root,root) %{prefix}/man/man1/make_smbcodepage.1
%attr(-,root,root) %{prefix}/man/man1/smbrun.1
%attr(-,root,root) %{prefix}/man/man1/smbtar.1
%attr(-,root,root) %{prefix}/man/man1/testparm.1
%attr(-,root,root) %{prefix}/man/man1/testprns.1
%attr(-,root,root) %{prefix}/man/man1/nmblookup.1
%attr(-,root,root) %{prefix}/man/man5/smb.conf.5
%attr(-,root,root) %{prefix}/man/man5/lmhosts.5
%attr(-,root,root) %{prefix}/man/man5/smbpasswd.5
%attr(-,root,root) %{prefix}/man/man7/samba.7
%attr(-,root,root) %{prefix}/man/man8/smbd.8
%attr(-,root,root) %{prefix}/man/man8/nmbd.8
%attr(-,root,root) %{prefix}/man/man8/smbpasswd.8
%attr(-,root,root) %{prefix}/man/man8/swat.8
%attr(-,root,root) %{prefix}/man/man8/smbmnt.8
%attr(-,root,root) %{prefix}/man/man8/smbmount.8
%attr(-,root,root) %{prefix}/man/man8/smbspool.8
%attr(-,root,root) %dir %{prefix}/lib/codepages
%attr(-,root,root) %dir %{prefix}/lib/codepages/src
%attr(-,root,root) %dir %{prefix}/var/locks
%attr(-,root,root) %dir %{prefix}/private
%attr(-,root,root) %{prefix}/bin/winbindd
%attr(-,root,root) %{prefix}/bin/samedit
%attr(-,root,root) /lib/libnss_winbind.so
%attr(-,root,root) /lib/security/pam_winbind.so
