/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2018-2022 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file plugin_ASTAnalyzer.cpp
 * @brief Plugin analyzing the Clang AST retrieving useful information for PandA
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "plugin_includes.hpp"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

#include <boost/algorithm/string/predicate.hpp>
#include <cstdlib>
#include <iostream>

static std::map<std::string, std::map<clang::SourceLocation, std::pair<std::string, std::string>>>
    HLS_interface_PragmaMap;
static std::map<std::string, std::map<clang::SourceLocation, std::pair<std::string, std::string>>>
    HLS_interface_PragmaMapArraySize;
static std::map<std::string, std::map<clang::SourceLocation, std::pair<std::string, std::string>>>
    HLS_interface_PragmaMapAttribute2;
static std::map<std::string, std::map<clang::SourceLocation, std::pair<std::string, std::string>>>
    HLS_interface_PragmaMapAttribute3;

static std::map<std::string, std::vector<clang::SourceLocation>> HLS_pipeline_PragmaMap;
static std::map<std::string, std::vector<clang::SourceLocation>> HLS_simple_pipeline_PragmaMap;
static std::map<std::string, std::map<clang::SourceLocation, std::string>> HLS_stallable_pipeline_PragmaMap;

namespace clang
{
   class FunctionArgConsumer : public clang::ASTConsumer
   {
      CompilerInstance& CI;
      std::string topfname;
      std::string outdir_name;
      std::string InFile;
      clang::PrintingPolicy pp;

      std::map<std::string, std::vector<std::string>> Fun2Params;
      std::map<std::string, std::vector<std::string>> Fun2ParamType;
      std::map<std::string, std::vector<std::string>> Fun2ParamTypeOrig;
      std::map<std::string, std::map<std::string, std::string>> Fun2ParamSize;
      std::map<std::string, std::map<std::string, std::string>> Fun2ParamAttribute2;
      std::map<std::string, std::map<std::string, std::string>> Fun2ParamAttribute3;
      std::map<std::string, std::vector<std::string>> Fun2ParamInclude;
      std::map<std::string, std::string> Fun2Demangled;
      std::map<std::string, clang::SourceLocation> prevLoc;

      std::map<std::string, std::vector<std::string>> HLS_interfaceMap;
      std::map<std::string, std::map<std::string, std::string>> HLS_interfaceArraySizeMap;
      std::set<std::string> HLS_pipelineSet;
      std::set<std::string> HLS_simple_pipelineSet;
      std::map<std::string, std::string> HLS_stallable_pipelineMap;

      std::string create_file_basename_string(const std::string& on, const std::string& original_filename)
      {
         std::size_t found = original_filename.find_last_of("/\\");
         std::string dump_base_name;
         if(found == std::string::npos)
         {
            dump_base_name = original_filename;
         }
         else
         {
            dump_base_name = original_filename.substr(found + 1);
         }
         return on + "/" + dump_base_name;
      }

      void convert_unescaped(std::string& ioString) const
      {
         std::string::size_type lPos = 0;
         while((lPos = ioString.find_first_of("&<>'\"", lPos)) != std::string::npos)
         {
            switch(ioString[lPos])
            {
               case '&':
                  ioString.replace(lPos++, 1, "&amp;");
                  break;
               case '<':
                  ioString.replace(lPos++, 1, "&lt;");
                  break;
               case '>':
                  ioString.replace(lPos++, 1, "&gt;");
                  break;
               case '\'':
                  ioString.replace(lPos++, 1, "&apos;");
                  break;
               case '"':
                  ioString.replace(lPos++, 1, "&quot;");
                  break;
               default:
               {
                  // Do nothing
               }
            }
         }
      }

      void writeXML_interfaceFile(const std::string& filename, const std::string& TopFunctionName) const
      {
         std::error_code EC;
#if __clang_major__ >= 7 && !defined(VVD)
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         stream << "<?xml version=\"1.0\"?>\n";
         stream << "<module>\n";
         for(auto funArgPair : Fun2Params)
         {
            if(!TopFunctionName.empty() && Fun2Demangled.find(funArgPair.first)->second != TopFunctionName &&
               funArgPair.first != TopFunctionName)
            {
               continue;
            }
            bool hasInterfaceType = HLS_interfaceMap.find(funArgPair.first) != HLS_interfaceMap.end();
            if(hasInterfaceType)
            {
               stream << "  <function id=\"" << funArgPair.first << "\">\n";
               const auto& interfaceTypeVec = HLS_interfaceMap.find(funArgPair.first)->second;
               const auto& interfaceTypenameVec = Fun2ParamType.find(funArgPair.first)->second;
               const auto& interfaceTypenameOrigVec = Fun2ParamTypeOrig.find(funArgPair.first)->second;
               const auto& interfaceTypenameIncludeVec = Fun2ParamInclude.find(funArgPair.first)->second;
               unsigned int ArgIndex = 0;
               for(const auto& par : funArgPair.second)
               {
                  std::string typenameArg = interfaceTypenameVec.at(ArgIndex);
                  std::string typenameOrigArg = interfaceTypenameOrigVec.at(ArgIndex);
                  convert_unescaped(typenameArg);
                  convert_unescaped(typenameOrigArg);
                  stream << "    <arg id=\"" << par << "\" interface_type=\"" << interfaceTypeVec.at(ArgIndex)
                         << "\" interface_typename=\"" << typenameArg << "\" interface_typename_orig=\""
                         << typenameOrigArg << "\"";
                  if(Fun2ParamSize.find(funArgPair.first) != Fun2ParamSize.end() &&
                     Fun2ParamSize.find(funArgPair.first)->second.find(par) !=
                         Fun2ParamSize.find(funArgPair.first)->second.end())
                     stream << " size=\"" << Fun2ParamSize.find(funArgPair.first)->second.find(par)->second << "\"";
                  if(Fun2ParamAttribute2.find(funArgPair.first) != Fun2ParamAttribute2.end() &&
                     Fun2ParamAttribute2.find(funArgPair.first)->second.find(par) !=
                         Fun2ParamAttribute2.find(funArgPair.first)->second.end())
                     stream << " attribute2=\"" << Fun2ParamAttribute2.find(funArgPair.first)->second.find(par)->second
                            << "\"";
                  if(Fun2ParamAttribute3.find(funArgPair.first) != Fun2ParamAttribute3.end() &&
                     Fun2ParamAttribute3.find(funArgPair.first)->second.find(par) !=
                         Fun2ParamAttribute3.find(funArgPair.first)->second.end())
                     stream << " attribute3=\"" << Fun2ParamAttribute3.find(funArgPair.first)->second.find(par)->second
                            << "\"";
                  stream << " interface_typename_include=\"" << interfaceTypenameIncludeVec.at(ArgIndex) << "\"/>\n";
                  ++ArgIndex;
               }
               stream << "  </function>\n";
            }
         }
         stream << "</module>\n";
      }

      void writeXML_pipelineFile(const std::string& filename, const std::string& TopFunctionName) const
      {
         std::error_code EC;
#if __clang_major__ >= 7 && !defined(VVD)
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         stream << "<?xml version=\"1.0\"?>\n";
         stream << "<module>\n";
         for(auto function : Fun2Params)
         {
            std::string function_name = function.first;
            std::string is_pipelined = "no";
            std::string simple_pipeline = "no";
            std::string initiation_time = "1";
            if(HLS_pipelineSet.find(function_name) != HLS_pipelineSet.end())
            {
               is_pipelined = "yes";
               if(HLS_simple_pipelineSet.find(function_name) != HLS_simple_pipelineSet.end())
               {
                  simple_pipeline = "yes";
               }
               else if(HLS_stallable_pipelineMap.find(function_name) != HLS_stallable_pipelineMap.end())
               {
                  initiation_time = HLS_stallable_pipelineMap.find(function_name)->second;
               }
               else
               {
                  DiagnosticsEngine& D = CI.getDiagnostics();
                  D.Report(
                      D.getCustomDiagID(DiagnosticsEngine::Error, "The defined pipeline is not simple nor stallable"));
               }
               stream << "  <function id=\"" << function_name << "\" is_pipelined=\"" << is_pipelined
                      << "\" is_simple=\"" << simple_pipeline << "\" initiation_time=\"" << initiation_time << "\"/>\n";
            }
         }
         stream << "</module>\n";
      }

      void writeFun2Params(const std::string& filename) const
      {
         std::error_code EC;
#if __clang_major__ >= 7 && !defined(VVD)
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         for(auto fun2parms_el : Fun2Params)
         {
            stream << fun2parms_el.first;
            for(auto par : fun2parms_el.second)
               stream << " " << par;
            stream << "\n";
         }
      }

      const NamedDecl* getBaseTypeDecl(const QualType& qt) const
      {
         const Type* ty = qt.getTypePtr();
         NamedDecl* ND = nullptr;

         if(ty->isPointerType() || ty->isReferenceType())
         {
            return getBaseTypeDecl(ty->getPointeeType());
         }
         if(ty->isRecordType())
         {
            ND = ty->getAs<RecordType>()->getDecl();
         }
         else if(ty->isEnumeralType())
         {
            ND = ty->getAs<EnumType>()->getDecl();
         }
         else if(ty->getTypeClass() == Type::Typedef)
         {
            ND = ty->getAs<TypedefType>()->getDecl();
         }
         else if(ty->isArrayType())
         {
            return getBaseTypeDecl(ty->castAsArrayTypeUnsafe()->getElementType());
         }
         return ND;
      }

      QualType RemoveTypedef(QualType t) const
      {
         if(t->getTypeClass() == Type::Typedef)
            return RemoveTypedef(t->getAs<TypedefType>()->getDecl()->getUnderlyingType());
         else if(t->getTypeClass() == Type::TemplateSpecialization)
         {
            return t;
         }
         else
            return t;
      }

      std::string GetTypeNameCanonical(const QualType& t, const PrintingPolicy& pp) const
      {
         auto typeName = t->getCanonicalTypeInternal().getAsString(pp);
         auto key = std::string("class ");
         auto constkey = std::string("const class ");
         if(typeName.find(key) == 0)
            typeName = typeName.substr(key.size());
         else if(typeName.find(constkey) == 0)
            typeName = typeName.substr(constkey.size());
         return typeName;
      }

      std::string getMangledName(const FunctionDecl* decl)
      {
         auto mangleContext = decl->getASTContext().createMangleContext();

         if(!mangleContext->shouldMangleDeclName(decl))
         {
            delete mangleContext;
            return decl->getNameInfo().getName().getAsString();
         }
         std::string mangledName;
         if(llvm::isa<CXXConstructorDecl>(decl) || llvm::isa<CXXDestructorDecl>(decl))
         {
            delete mangleContext;
            return decl->getNameInfo().getName().getAsString();
            ;
         }
         llvm::raw_string_ostream ostream(mangledName);
         if(mangleContext->shouldMangleCXXName(decl))
         {
            mangleContext->mangleCXXName(decl, ostream);
            ostream.flush();
            delete mangleContext;
            return mangledName;
         }
         mangleContext->mangleName(decl, ostream);
         ostream.flush();
         delete mangleContext;
         return mangledName;
      }

      void AnalyzeFunctionDecl(const FunctionDecl* FD)
      {
         auto& SM = FD->getASTContext().getSourceManager();
         std::map<std::string, std::string> interface_PragmaMap;
         std::map<std::string, std::string> interface_PragmaMapArraySize;
         std::map<std::string, std::string> interface_PragmaMapAttribute2;
         std::map<std::string, std::string> interface_PragmaMapAttribute3;
         auto locEnd = FD->getSourceRange().getEnd();
         auto filename = SM.getPresumedLoc(locEnd, false).getFilename();
         if(HLS_interface_PragmaMap.find(filename) != HLS_interface_PragmaMap.end())
         {
            SourceLocation prev;
            if(prevLoc.find(filename) != prevLoc.end())
            {
               prev = prevLoc.find(filename)->second;
            }
            for(auto& loc2pair : HLS_interface_PragmaMap.find(filename)->second)
            {
               if((prev.isInvalid() || prev < loc2pair.first) && (loc2pair.first < locEnd))
               {
                  interface_PragmaMap[loc2pair.second.first] = loc2pair.second.second;
               }
            }
            if(HLS_interface_PragmaMapArraySize.find(filename) != HLS_interface_PragmaMapArraySize.end())
            {
               for(auto& loc2pair : HLS_interface_PragmaMapArraySize.find(filename)->second)
               {
                  if((prev.isInvalid() || prev < loc2pair.first) && (loc2pair.first < locEnd))
                  {
                     interface_PragmaMapArraySize[loc2pair.second.first] = loc2pair.second.second;
                  }
               }
            }
            if(HLS_interface_PragmaMapAttribute2.find(filename) != HLS_interface_PragmaMapAttribute2.end())
            {
               for(auto& loc2pair : HLS_interface_PragmaMapAttribute2.find(filename)->second)
               {
                  if((prev.isInvalid() || prev < loc2pair.first) && (loc2pair.first < locEnd))
                  {
                     interface_PragmaMapAttribute2[loc2pair.second.first] = loc2pair.second.second;
                  }
               }
            }
            if(HLS_interface_PragmaMapAttribute3.find(filename) != HLS_interface_PragmaMapAttribute3.end())
            {
               for(auto& loc2pair : HLS_interface_PragmaMapAttribute3.find(filename)->second)
               {
                  if((prev.isInvalid() || prev < loc2pair.first) && (loc2pair.first < locEnd))
                  {
                     interface_PragmaMapAttribute3[loc2pair.second.first] = loc2pair.second.second;
                  }
               }
            }
         }

         if(!FD->isVariadic() && FD->hasBody())
         {
            auto funName = getMangledName(FD);
            Fun2Demangled[funName] = FD->getNameInfo().getName().getAsString();
            // llvm::errs()<<"funName:"<<funName<<"\n";
            auto par_index = 0u;
            for(const auto par : FD->parameters())
            {
               if(const ParmVarDecl* ND = dyn_cast<ParmVarDecl>(par))
               {
                  std::string interfaceType = "default";
                  std::string arraySize;
                  std::string attribute2;
                  std::string attribute3;
                  std::string UserDefinedInterfaceType;
                  std::string ParamTypeName;
                  std::string ParamTypeNameOrig;
                  std::string ParamTypeInclude;
                  auto parName = ND->getNameAsString();
                  bool UDIT_p = false;
                  bool UDIT_attribute2_p = false;
                  bool UDIT_attribute3_p = false;
                  if(parName.empty())
                  {
                     parName = "P" + std::to_string(par_index);
                  }

                  if(interface_PragmaMap.find(parName) != interface_PragmaMap.end())
                  {
                     UserDefinedInterfaceType = interface_PragmaMap.find(parName)->second;
                     UDIT_p = true;
                     if(interface_PragmaMapAttribute2.find(parName) != interface_PragmaMapAttribute2.end())
                     {
                        attribute2 = interface_PragmaMapAttribute2.find(parName)->second;
                        UDIT_attribute2_p = true;
                     }
                     if(interface_PragmaMapAttribute3.find(parName) != interface_PragmaMapAttribute3.end())
                     {
                        attribute3 = interface_PragmaMapAttribute3.find(parName)->second;
                        UDIT_attribute3_p = true;
                     }
                  }
                  auto argType = ND->getType();
                  auto manageArray = [&](const ConstantArrayType* CA, bool setInterfaceType) {
                     auto OrigTotArraySize = CA->getSize();
                     std::string Dimensions;
                     if(!setInterfaceType)
                     {
                        Dimensions = "[" + OrigTotArraySize.toString(10, false) + "]";
                     }
                     while(CA->getElementType()->isConstantArrayType())
                     {
                        CA = cast<ConstantArrayType>(CA->getElementType());
                        auto n_el = CA->getSize();
                        Dimensions = Dimensions + "[" + n_el.toString(10, false) + "]";
                        OrigTotArraySize *= n_el;
                     }
                     auto paramTypeRemTD = RemoveTypedef(CA->getElementType());
                     ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp) + " *";
                     ParamTypeNameOrig =
                         paramTypeRemTD.getAsString(pp) + (Dimensions == "" ? " *" : " (*)" + Dimensions);
                     if(auto BTD = getBaseTypeDecl(paramTypeRemTD))
                        ParamTypeInclude = SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                     if(setInterfaceType)
                     {
                        interfaceType = "array";
                        arraySize = OrigTotArraySize.toString(10, false);
                        assert(arraySize != "0");
                     }
                  };
                  // argType->dump ();
                  if(isa<DecayedType>(argType))
                  {
                     auto DT = cast<DecayedType>(argType);
                     if(DT->getOriginalType().IgnoreParens()->isConstantArrayType())
                     {
                        manageArray(llvm::cast<ConstantArrayType>(DT->getOriginalType().IgnoreParens()), true);
                     }
                     else
                     {
                        auto paramTypeRemTD = RemoveTypedef(argType);
                        ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp);
                        ParamTypeNameOrig = paramTypeRemTD.getAsString(pp);
                        if(auto BTD = getBaseTypeDecl(paramTypeRemTD))
                           ParamTypeInclude = SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                     }
                     if(UDIT_p)
                     {
                        if(UserDefinedInterfaceType != "handshake" && UserDefinedInterfaceType != "fifo" &&
                           UserDefinedInterfaceType.find("array") == std::string::npos &&
                           UserDefinedInterfaceType != "bus" && UserDefinedInterfaceType != "m_axi" &&
                           UserDefinedInterfaceType != "axis")
                        {
                           DiagnosticsEngine& D = CI.getDiagnostics();
                           D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                      "#pragma HLS_interface non-consistent with parameter of constant "
                                                      "array type, where user defined interface is: %0"))
                               .AddString(UserDefinedInterfaceType);
                        }
                        else
                        {
                           interfaceType = UserDefinedInterfaceType;
                           if(interfaceType == "array")
                           {
                              if(interface_PragmaMapArraySize.find(parName) == interface_PragmaMapArraySize.end())
                              {
                                 DiagnosticsEngine& D = CI.getDiagnostics();
                                 D.Report(D.getCustomDiagID(
                                              DiagnosticsEngine::Error,
                                              "#pragma HLS_interface inconsistent internal data structure: %0"))
                                     .AddString(UserDefinedInterfaceType);
                              }
                              else
                                 arraySize = interface_PragmaMapArraySize.find(parName)->second;
                           }
                        }
                     }
                  }
                  else if(argType->isPointerType() || argType->isReferenceType())
                  {
                     if(auto PT = llvm::dyn_cast<PointerType>(argType))
                     {
                        if(auto CA = llvm::dyn_cast<ConstantArrayType>(PT->getPointeeType().IgnoreParens()))
                        {
                           manageArray(CA, false);
                        }
                        else
                        {
                           auto paramTypeRemTD = RemoveTypedef(PT->getPointeeType());
                           ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp) + " *";
                           ParamTypeNameOrig = paramTypeRemTD.getAsString(pp) + " *";
                           if(auto BTD = getBaseTypeDecl(paramTypeRemTD))
                              ParamTypeInclude =
                                  SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                        }
                     }
                     else if(auto RT = dyn_cast<ReferenceType>(argType))
                     {
                        auto paramTypeRemTD = RemoveTypedef(RT->getPointeeType());
                        ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp) + " &";
                        ParamTypeNameOrig = paramTypeRemTD.getAsString(pp) + " &";
                        if(auto BTD = getBaseTypeDecl(paramTypeRemTD))
                           ParamTypeInclude = SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                     }
                     else
                     {
                        auto paramTypeRemTD = RemoveTypedef(argType);
                        ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp);
                        ParamTypeNameOrig = paramTypeRemTD.getAsString(pp);
                        if(auto BTD = getBaseTypeDecl(paramTypeRemTD))
                           ParamTypeInclude = SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                     }
                     interfaceType = "ptrdefault";
                     if(UDIT_p)
                     {
                        if(UserDefinedInterfaceType != "none" && UserDefinedInterfaceType != "none_registered" &&
                           UserDefinedInterfaceType != "handshake" && UserDefinedInterfaceType != "valid" &&
                           UserDefinedInterfaceType != "ovalid" && UserDefinedInterfaceType != "acknowledge" &&
                           UserDefinedInterfaceType != "fifo" && UserDefinedInterfaceType != "bus" &&
                           UserDefinedInterfaceType != "m_axi" && UserDefinedInterfaceType != "axis")
                        {
                           DiagnosticsEngine& D = CI.getDiagnostics();
                           D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                      "#pragma HLS_interface non-consistent with parameter of pointer "
                                                      "type, where user defined interface is: %0"))
                               .AddString(UserDefinedInterfaceType);
                        }
                        else
                        {
                           interfaceType = UserDefinedInterfaceType;
                        }
                     }
                  }
                  else
                  {
                     auto paramTypeRemTD = RemoveTypedef(argType);
                     ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp);
                     ParamTypeNameOrig = paramTypeRemTD.getAsString(pp);
                     if(auto BTD = getBaseTypeDecl(paramTypeRemTD))
                        ParamTypeInclude = SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                     if(!argType->isBuiltinType() && !argType->isEnumeralType())
                     {
                        interfaceType = "none";
                     }
                     if(UDIT_p)
                     {
                        if(UserDefinedInterfaceType != "none" && UserDefinedInterfaceType != "none_registered" &&
                           UserDefinedInterfaceType != "handshake" && UserDefinedInterfaceType != "valid" &&
                           UserDefinedInterfaceType != "ovalid" && UserDefinedInterfaceType != "acknowledge")
                        {
                           DiagnosticsEngine& D = CI.getDiagnostics();
                           D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                      "#pragma HLS_interface non-consistent with parameter of builtin "
                                                      "type, where user defined interface is: %0"))
                               .AddString(UserDefinedInterfaceType);
                        }
                        else
                        {
                           interfaceType = UserDefinedInterfaceType;
                        }
                        if((argType->isBuiltinType() || argType->isEnumeralType()) && interfaceType == "none")
                        {
                           interfaceType = "default";
                        }
                     }
                  }
                  HLS_interfaceMap[funName].push_back(interfaceType);
                  Fun2Params[funName].push_back(parName);
                  Fun2ParamType[funName].push_back(ParamTypeName);
                  Fun2ParamTypeOrig[funName].push_back(ParamTypeNameOrig);
                  if(interfaceType == "array")
                     Fun2ParamSize[funName][parName] = arraySize;
                  if(interfaceType == "m_axi" && UDIT_attribute2_p)
                     Fun2ParamAttribute2[funName][parName] = attribute2;
                  if((interfaceType == "m_axi" || interfaceType == "array") && UDIT_attribute3_p)
                     Fun2ParamAttribute3[funName][parName] = attribute3;
                  Fun2ParamInclude[funName].push_back(ParamTypeInclude);
               }
               ++par_index;
            }

            if(HLS_pipeline_PragmaMap.find(filename) != HLS_pipeline_PragmaMap.end())
            {
               SourceLocation prev;
               if(prevLoc.find(filename) != prevLoc.end())
               {
                  prev = prevLoc.find(filename)->second;
               }
               for(auto& loc : HLS_pipeline_PragmaMap.find(filename)->second)
               {
                  if((prev.isInvalid() || prev < loc) && (loc < locEnd))
                  {
                     HLS_pipelineSet.insert(funName);
                  }
               }
               if(HLS_simple_pipeline_PragmaMap.find(filename) != HLS_simple_pipeline_PragmaMap.end())
               {
                  if(prevLoc.find(filename) != prevLoc.end())
                  {
                     prev = prevLoc.find(filename)->second;
                  }
                  for(auto& loc : HLS_simple_pipeline_PragmaMap.find(filename)->second)
                  {
                     if((prev.isInvalid() || prev < loc) && (loc < locEnd))
                     {
                        HLS_simple_pipelineSet.insert(funName);
                     }
                  }
               }
               else if(HLS_stallable_pipeline_PragmaMap.find(filename) != HLS_stallable_pipeline_PragmaMap.end())
               {
                  if(prevLoc.find(filename) != prevLoc.end())
                  {
                     prev = prevLoc.find(filename)->second;
                  }
                  for(auto& loc_pair : HLS_stallable_pipeline_PragmaMap.find(filename)->second)
                  {
                     if((prev.isInvalid() || prev < loc_pair.first) && (loc_pair.first < locEnd))
                     {
                        HLS_stallable_pipelineMap[funName] = loc_pair.second;
                     }
                  }
               }
               else
               {
                  DiagnosticsEngine& D = CI.getDiagnostics();
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "Pipeline parser has an error"));
               }
            }
         }
      }

    public:
      FunctionArgConsumer(CompilerInstance& Instance, const std::string& _topfname, const std::string& _outdir_name,
                          std::string _InFile, const clang::PrintingPolicy& _pp)
          : CI(Instance), topfname(_topfname), outdir_name(_outdir_name), InFile(_InFile), pp(_pp)
      {
      }

      bool HandleTopLevelDecl(DeclGroupRef DG) override
      {
         for(auto D : DG)
         {
            if(const auto* FD = dyn_cast<FunctionDecl>(D))
            {
               AnalyzeFunctionDecl(FD);
               auto endLoc = FD->getSourceRange().getEnd();
               auto& SM = FD->getASTContext().getSourceManager();
               auto filename = SM.getPresumedLoc(endLoc, false).getFilename();
               prevLoc[filename] = endLoc;
            }
            else if(const auto* LSD = dyn_cast<LinkageSpecDecl>(D))
            {
               for(auto d : LSD->decls())
               {
                  if(const FunctionDecl* fd = dyn_cast<FunctionDecl>(d))
                  {
                     AnalyzeFunctionDecl(fd);
                     auto endLoc = fd->getSourceRange().getEnd();
                     auto& SM = fd->getASTContext().getSourceManager();
                     auto filename = SM.getPresumedLoc(endLoc, false).getFilename();
                     prevLoc[filename] = endLoc;
                  }
               }
            }
         }
         return true;
      }

      void HandleTranslationUnit(ASTContext&) override
      {
         auto baseFilename = create_file_basename_string(outdir_name, InFile);
         std::string interface_fun2parms_filename = baseFilename + ".params.txt";
         std::string interface_XML_filename = baseFilename + ".interface.xml";
         std::string pipeline_XML_filename = baseFilename + ".pipeline.xml";
         writeFun2Params(interface_fun2parms_filename);
         writeXML_interfaceFile(interface_XML_filename, topfname);
         writeXML_pipelineFile(pipeline_XML_filename, topfname);
      }
   };

   class HLS_interface_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_interface_PragmaHandler() : PragmaHandler("HLS_interface")
      {
      }

      void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                        PragmaIntroducer
#else
                        PragmaIntroducerKind
#endif
                        /*Introducer*/,
                        Token& PragmaTok) override
      {
         Token Tok{};
         unsigned int index = 0;
         std::string par;
         std::string interface;
         std::string ArraySize;
         std::string Attribute2;
         std::string Attribute3;
         bool bundle_p = false;
         bool equal_p = false;
         auto loc = PragmaTok.getLocation();
         while(Tok.isNot(tok::eod))
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               if(index == 0)
               {
                  par = PP.getSpelling(Tok);
               }
               else if(index >= 1)
               {
                  auto tokString = PP.getSpelling(Tok);
                  if(index == 1)
                  {
                     if(tokString != "none" && tokString != "none_registered" && tokString != "array" &&
                        tokString != "bus" && tokString != "fifo" && tokString != "handshake" && tokString != "valid" &&
                        tokString != "ovalid" && tokString != "acknowledge" && tokString != "m_axi" &&
                        tokString != "axis")
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(
                            DiagnosticsEngine::Error,
                            "#pragma HLS_interface unexpected interface type. Currently accepted keywords are: "
                            "none,none_registered,array,bus,fifo,handshake,valid,ovalid,acknowledge");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                     interface += tokString;
                  }
                  else if(index == 2)
                  {
                     if((Tok.isNot(tok::numeric_constant) && interface == "array") ||
                        ((tokString != "direct" && tokString != "axi_slave" && tokString != "bundle") &&
                         interface == "m_axi") ||
                        (interface != "array" && interface != "m_axi"))
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed1");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                     if(interface == "array")
                        ArraySize = tokString;
                     else if(interface == "m_axi" && tokString == "bundle")
                     {
                        bundle_p = true;
                     }
                     else if(interface == "m_axi")
                        Attribute2 = tokString;
                  }
                  else if(index == 3)
                  {
                     if(interface == "m_axi" && tokString == "=" && bundle_p)
                        equal_p = true;
                     else if((interface == "m_axi" || interface == "array") && tokString == "bundle")
                     {
                        bundle_p = true;
                     }
                     else
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed2");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                  }
                  else if(index == 4)
                  {
                     if((interface == "m_axi" || interface == "array") && bundle_p && equal_p)
                        Attribute3 = tokString;
                     else if((interface == "m_axi" || interface == "array") && tokString == "=" && bundle_p)
                        equal_p = true;
                     else
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed2");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                  }
                  else if(index == 5)
                  {
                     if((interface == "m_axi" || interface == "array") && bundle_p && equal_p)
                        Attribute3 = tokString;
                     else
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed2");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                  }
               }
               ++index;
            }
         }
         if(index >= 2)
         {
            auto& SM = PP.getSourceManager();
            auto filename = SM.getPresumedLoc(loc, false).getFilename();
            if(ArraySize != "" && ArraySize != "0" && interface != "array" && interface != "m_axi")
            {
               DiagnosticsEngine& D = PP.getDiagnostics();
               unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed");
               D.Report(PragmaTok.getLocation(), ID);
            }
            HLS_interface_PragmaMap[filename][loc] = std::make_pair(par, interface);
            if(interface == "array")
            {
               HLS_interface_PragmaMapArraySize[filename][loc] = std::make_pair(par, ArraySize);
            }
            if(interface == "m_axi" && Attribute2 != "")
            {
               HLS_interface_PragmaMapAttribute2[filename][loc] = std::make_pair(par, Attribute2);
            }
            if((interface == "m_axi" || interface == "array") && Attribute3 != "")
            {
               HLS_interface_PragmaMapAttribute3[filename][loc] = std::make_pair(par, Attribute3);
            }
         }
         else
         {
            DiagnosticsEngine& D = PP.getDiagnostics();
            unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed");
            D.Report(PragmaTok.getLocation(), ID);
         }
      }
   };

   class HLS_simple_pipeline_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_simple_pipeline_PragmaHandler() : PragmaHandler("HLS_simple_pipeline")
      {
      }

      void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                        PragmaIntroducer
#else
                        PragmaIntroducerKind
#endif
                        /*Introducer*/,
                        Token& PragmaTok) override
      {
         Token Tok{};
         auto loc = PragmaTok.getLocation();
         auto& SM = PP.getSourceManager();
         auto filename = SM.getPresumedLoc(loc, false).getFilename();

         while(Tok.isNot(tok::eod))
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               DiagnosticsEngine& D = PP.getDiagnostics();
               unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_pipeline malformed");
               D.Report(PragmaTok.getLocation(), ID);
            }
         }
         HLS_pipeline_PragmaMap[filename].push_back(loc);
         HLS_simple_pipeline_PragmaMap[filename].push_back(loc);
      }
   };

   class HLS_stallable_pipeline_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_stallable_pipeline_PragmaHandler() : PragmaHandler("HLS_stallable_pipeline")
      {
      }

      void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                        PragmaIntroducer
#else
                        PragmaIntroducerKind
#endif
                        /*Introducer*/,
                        Token& PragmaTok) override
      {
         Token Tok{};
         auto loc = PragmaTok.getLocation();
         auto& SM = PP.getSourceManager();
         auto filename = SM.getPresumedLoc(loc, false).getFilename();
         std::string time;
         int index = 0;
         while(Tok.isNot(tok::eod))
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               auto tokString = PP.getSpelling(Tok);
               if(index == 0)
               {
                  time = tokString;
                  if(Tok.isNot(tok::numeric_constant))
                  {
                     DiagnosticsEngine& D = PP.getDiagnostics();
                     unsigned ID =
                         D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_stallable_pipeline malformed");
                     D.Report(PragmaTok.getLocation(), ID);
                  }
               }
               else
               {
                  DiagnosticsEngine& D = PP.getDiagnostics();
                  unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_stallable_pipeline malformed");
                  D.Report(PragmaTok.getLocation(), ID);
               }
               ++index;
            }
         }
         HLS_pipeline_PragmaMap[filename].push_back(loc);
         HLS_stallable_pipeline_PragmaMap[filename][loc] = time;
      }
   };

   class CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) : public PluginASTAction
   {
      std::string topfname;
      std::string outdir_name;
      bool cppflag;

    protected:
      std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, llvm::StringRef InFile) override
      {
         DiagnosticsEngine& D = CI.getDiagnostics();
         if(outdir_name.empty())
         {
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "outputdir argument not specified"));
         }
         clang::Preprocessor& PP = CI.getPreprocessor();
         PP.AddPragmaHandler(new HLS_interface_PragmaHandler());
         PP.AddPragmaHandler(new HLS_simple_pipeline_PragmaHandler());
         PP.AddPragmaHandler(new HLS_stallable_pipeline_PragmaHandler());
         auto pp = clang::PrintingPolicy(clang::LangOptions());
         if(cppflag)
         {
            pp.adjustForCPlusPlus();
         }
#if __clang_major__ > 9
         return std::make_unique<FunctionArgConsumer>(CI, topfname, outdir_name, InFile.data(), pp);
#else
         return llvm::make_unique<FunctionArgConsumer>(CI, topfname, outdir_name, InFile, pp);
#endif
      }

      bool ParseArgs(const CompilerInstance& CI, const std::vector<std::string>& args) override
      {
         DiagnosticsEngine& D = CI.getDiagnostics();
         for(size_t i = 0, e = args.size(); i != e; ++i)
         {
            if(args.at(i) == "-topfname")
            {
               if(i + 1 >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing topfname argument"));
                  return false;
               }
               ++i;
               topfname = args.at(i);
            }
            else if(args.at(i) == "-outputdir")
            {
               if(i + 1 >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing outputdir argument"));
                  return false;
               }
               ++i;
               outdir_name = args.at(i);
            }
            else if(args.at(i) == "-cppflag")
            {
               if(i + 1 >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing cppflag argument"));
                  return false;
               }
               ++i;
               cppflag = std::atoi(args.at(i).data()) == 1;
            }
         }
         if(!args.empty() && args.at(0) == "-help")
         {
            PrintHelp(llvm::errs());
         }

         if(outdir_name.empty())
         {
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "outputdir not specified"));
         }
         return true;
      }

      void PrintHelp(llvm::raw_ostream& ros)
      {
         ros << "Help for " CLANG_VERSION_STRING(_plugin_ASTAnalyzer) " plugin\n";
         ros << "-outputdir <directory>\n";
         ros << "  Directory where the raw file will be written\n";
         ros << "-cppflag <type>\n";
         ros << "  1 if input source file is C++, 0 else\n";
         ros << "-topfname <function name>\n";
         ros << "  Function from which the Point-To analysis has to start\n";
      }

      PluginASTAction::ActionType getActionType() override
      {
         return AddAfterMainAction;
      }

    public:
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)() : cppflag(false)
      {
      }
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & step) = delete;
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & operator=(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) &) = delete;
   };

#ifdef _WIN32

   void initializeplugin_ASTAnalyzer()
   {
      static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)> X(
          CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
   }
#endif
} // namespace clang

#ifndef _WIN32
static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)>
    X(CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
#endif
